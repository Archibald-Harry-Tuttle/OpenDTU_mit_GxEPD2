// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_config.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiConfigClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = server;

    _server->on("/api/config/get", HTTP_GET, std::bind(&WebApiConfigClass::onConfigGet, this, _1));
    _server->on("/api/config/delete", HTTP_POST, std::bind(&WebApiConfigClass::onConfigDelete, this, _1));
    _server->on("/api/config/list", HTTP_GET, std::bind(&WebApiConfigClass::onConfigListGet, this, _1));
    _server->on("/api/config/upload", HTTP_POST,
        std::bind(&WebApiConfigClass::onConfigUploadFinish, this, _1),
        std::bind(&WebApiConfigClass::onConfigUpload, this, _1, _2, _3, _4, _5, _6));
}

void WebApiConfigClass::loop()
{
}

void WebApiConfigClass::onConfigGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    String requestFile = CONFIG_FILENAME;
    if (request->hasParam("file")) {
        String name = "/" + request->getParam("file")->value();
        if (LittleFS.exists(name)) {
            requestFile = name;
        } else {
            request->send(404);
        }
    }

    request->send(LittleFS, requestFile, String(), true);
}

void WebApiConfigClass::onConfigDelete(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("delete"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("delete")].as<bool>() == false) {
        retMsg[F("message")] = F("Not deleted anything!");
        retMsg[F("code")] = WebApiError::ConfigNotDeleted;
        response->setLength();
        request->send(response);
        return;
    }

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Configuration resettet. Rebooting now...");
    retMsg[F("code")] = WebApiError::ConfigSuccess;

    response->setLength();
    request->send(response);

    LittleFS.remove(CONFIG_FILENAME);
    ESP.restart();
}

void WebApiConfigClass::onConfigListGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    JsonArray data = root.createNestedArray(F("configs"));

    File rootfs = LittleFS.open("/");
    File file = rootfs.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            continue;
        }
        JsonObject obj = data.createNestedObject();
        obj["name"] = String(file.name());

        file = rootfs.openNextFile();
    }
    file.close();

    response->setLength();
    request->send(response);
}

void WebApiConfigClass::onConfigUploadFinish(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response

    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

void WebApiConfigClass::onConfigUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    if (!index) {
        // open the file on first call and store the file handle in the request object
        if (!request->hasParam("file")) {
            request->send(500);
            return;
        }
        String name = "/" + request->getParam("file")->value();
        request->_tempFile = LittleFS.open(name, "w");
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
    }

    if (final) {
        // close the file handle as the upload is now done
        request->_tempFile.close();
    }
}