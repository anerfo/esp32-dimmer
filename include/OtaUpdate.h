#include <WiFi.h>
#include <Update.h>
#include "cJSON.h"
#include "HTTPClient.h"

#if __has_include("myconfig.h")
  #include "myconfig.h"
#else
  #warning "Using Defaults: Copy myconfig.sample.h to myconfig.h and edit that to use your own settings"
  #include "myconfig.sample.h"
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION 0.1
#endif

#ifndef UPDATE_SERVER_PORT
#define UPDATE_SERVER_PORT 80
#endif

#ifndef UPDATE_JSON
#define UPDATE_JSON "update.json"
#endif

bool OtaChecked = false;

String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic 
void execOTA() {
  WiFiClient client;
  long contentLength = 0;
  bool isValidContentType = false;

  Serial.println("Connecting to: " + String(UPDATE_HOST));

  // Connect to S3
  if (client.connect(UPDATE_HOST, UPDATE_SERVER_PORT)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(UPDATE_JSON));

    HTTPClient http;

    String request = String("http://") + String(UPDATE_HOST) + String(":") + String(UPDATE_SERVER_PORT) + String(UPDATE_JSON);
    // Send request
    http.begin(request);
    http.GET();

    // Print the response
    String jsonContent = http.getString();
    Serial.print(jsonContent);

    // Disconnect
    http.end();
    cJSON *json = cJSON_Parse(jsonContent.c_str());
    String updateBinary;
    if(json == NULL)
    {
        Serial.println("downloaded file is not a valid json, aborting...\n");
        Serial.println(jsonContent);
        Serial.println(request);
        return;
    }
    else {	
        cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
        cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
        double new_version = version->valuedouble;
        if(!cJSON_IsNumber(version))
        {
            Serial.println("unable to read new version, aborting...\n");
            return;
        }
        else if(new_version <= FIRMWARE_VERSION ) {
            Serial.printf("current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
            return;
        }
        updateBinary = file->valuestring;
    }

    Serial.println("Fetching Bin: " + String(updateBinary));

    // Get the contents of the bin file
    client.print(String("GET ") + updateBinary + " HTTP/1.1\r\n" +
                 "Host: " + String(UPDATE_HOST) + ":" + String(UPDATE_SERVER_PORT) + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        return;
      }
    }
 
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          Serial.println(line);
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(UPDATE_HOST) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}

void checkOta(){
  if(OtaChecked)return; //Needs a complete reset to try an update again
  OtaChecked = true;
  execOTA();
}