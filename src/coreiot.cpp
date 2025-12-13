#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config_coreiot.h"
#include "mainserver.h"

WiFiClient mqttClient;
PubSubClient client(mqttClient);

unsigned long lastReconnectAttempt = 0;
String topicCommand;
String topicTelemetry;

// ✅ Forward declarations
void setLEDFromRPC(int ledNum, bool state, int brightness);
bool getLEDStateFromRPC(int ledNum);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("\n📩 MQTT RPC [%s] => ", topic);
    
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    
    // ✅ Xử lý RPC commands từ CoreIOT
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.println("❌ JSON parse error: " + String(error.c_str()));
        return;
    }
    
    // ✅ CoreIOT RPC format: {"method":"setState","params":true} hoặc {"method":"setState","params":50}
    String method = doc["method"] | "";
    
    if (method == "setState") {
        JsonVariant params = doc["params"];
        
        // ✅ Xử lý cả boolean (từ Power button) và number (từ Slider)
        if (params.is<bool>()) {
            // Power button: true/false
            bool state = params.as<bool>();
            Serial.printf("🎛️ RPC setState(%s)\n", state ? "true" : "false");
            setLEDFromRPC(1, state, state ? 100 : 0);  // LED 1 (GPIO 48)
        } 
        else if (params.is<int>() || params.is<float>()) {
            // Slider: 0-100
            int brightness = params.as<int>();
            brightness = constrain(brightness, 0, 100);
            bool state = (brightness > 0);
            
            Serial.printf("🎛️ RPC setState(%d%%) - LED %s\n", brightness, state ? "ON" : "OFF");
            setLEDFromRPC(1, state, brightness);  // LED 1 với brightness
        }
        
        // ✅ Gửi response về CoreIOT
        String response = "{\"result\":true}";
        client.publish((coreiot_username + "/rpc/response").c_str(), response.c_str());
        Serial.println("✅ RPC response sent");
    }
    else if (method == "getState") {
        Serial.println("🎛️ RPC getState()");
        
        // ✅ Trả về brightness (0-100) thay vì chỉ true/false
        int brightness = 0;
        if (led1.isOn) {
            brightness = led1.brightness;  // Trả về độ sáng hiện tại
        }
        
        // ✅ Gửi response về CoreIOT (slider cần giá trị số)
        String response = "{\"result\":" + String(brightness) + "}";
        client.publish((coreiot_username + "/rpc/response").c_str(), response.c_str());
        Serial.println("✅ RPC response sent: " + String(brightness) + "%");
    }
    else if (method == "setValue") {
        // ✅ Switch Control: setValue(true/false)
        bool state = doc["params"] | false;
        Serial.printf("🎛️ RPC setValue(%s) - Switch Control\n", state ? "true" : "false");
        setLEDFromRPC(1, state, state ? 100 : 0);  // LED 1
        
        // ✅ Gửi response về CoreIOT
        String response = "{\"result\":true}";
        client.publish((coreiot_username + "/rpc/response").c_str(), response.c_str());
        Serial.println("✅ RPC response sent");
    }
    else if (method == "getValue") {
        // ✅ Switch Control: getValue() - trả về true/false
        Serial.println("🎛️ RPC getValue() - Switch Control");
        bool state = getLEDStateFromRPC(1);  // LED 1
        
        // ✅ Gửi response về CoreIOT (Switch cần true/false)
        String response = "{\"result\":" + String(state ? "true" : "false") + "}";
        client.publish((coreiot_username + "/rpc/response").c_str(), response.c_str());
        Serial.println("✅ RPC response sent: " + String(state ? "ON" : "OFF"));
    }
    else {
        Serial.println("⚠️ Unknown RPC method: " + method);
    }
}

bool mqttReconnect() {
    // ✅ Validate config đầy đủ
    if (coreiot_server == "" || coreiot_port == 0) {
        static bool logged = false;
        if (!logged) {
            Serial.println("❌ CoreIOT: Thiếu server/port");
            logged = true;
        }
        return false;
    }

    if (coreiot_client_id == "" || coreiot_username == "") {
        static bool logged = false;
        if (!logged) {
            Serial.println("❌ CoreIOT: Thiếu Client ID hoặc Username");
            Serial.println("💡 Vui lòng vào Settings để cấu hình");
            logged = true;
        }
        return false;
    }

    // ✅ Check WiFi - FIXED for dual mode (AP+STA)
    if (!WiFi.isConnected()) {
        static bool logged = false;
        if (!logged) {
            Serial.println("⚠️ WiFi not connected, cannot connect MQTT");
            logged = true;
        }
        return false;
    }

    Serial.println("\n========================================");
    Serial.printf("🔌 MQTT connecting to %s:%d\n", coreiot_server.c_str(), coreiot_port);

    // ✅ Setup MQTT
    client.setServer(coreiot_server.c_str(), coreiot_port);
    client.setCallback(mqttCallback);

    // ✅ Topic for ThingsBoard-style telemetry
    topicCommand = coreiot_username + "/commands";
    topicTelemetry = "v1/devices/me/telemetry";

    Serial.println("📋 MQTT Credentials:");
    Serial.println("   Client ID: " + coreiot_client_id);
    Serial.println("   Username: " + coreiot_username);
    Serial.println("   Password: " + String(coreiot_password.length() > 0 ? "***" : "(empty)"));
    Serial.println("   Command: " + topicCommand);
    Serial.println("   Telemetry: " + topicTelemetry);

    // ✅ MQTT BASIC AUTHENTICATION
    bool connected = false;
    
    if (coreiot_password.length() > 0) {
        // Có password
        connected = client.connect(coreiot_client_id.c_str(), 
                                   coreiot_username.c_str(), 
                                   coreiot_password.c_str(), 
                                   NULL, 0, false, NULL);
    } else {
        // Không có password (anonymous with username)
        Serial.println("⚠️ Connecting without password...");
        connected = client.connect(coreiot_client_id.c_str(), 
                                   coreiot_username.c_str(), 
                                   "", 
                                   NULL, 0, false, NULL);
    }

    if (connected) {
        Serial.println("✅ MQTT connected!");
        
        // ✅ Subscribe to commands
        if (client.subscribe(topicCommand.c_str())) {
            Serial.println("✅ Subscribed: " + topicCommand);
        } else {
            Serial.println("⚠️ Subscribe failed: " + topicCommand);
        }
        
        // ✅ Subscribe to RPC requests (try multiple patterns)
        String rpcTopic1 = coreiot_username + "/rpc/request";
        String rpcTopic2 = "v1/devices/me/rpc/request/+";
        String rpcTopic3 = coreiot_client_id + "/rpc/request";
        String rpcTopic4 = coreiot_username + "/#";  // Wildcard - catch ALL messages
        
        if (client.subscribe(rpcTopic1.c_str())) {
            Serial.println("✅ Subscribed RPC: " + rpcTopic1);
        } else {
            Serial.println("⚠️ RPC Subscribe failed: " + rpcTopic1);
        }
        
        if (client.subscribe(rpcTopic2.c_str())) {
            Serial.println("✅ Subscribed RPC: " + rpcTopic2);
        } else {
            Serial.println("⚠️ RPC Subscribe failed: " + rpcTopic2);
        }
        
        if (client.subscribe(rpcTopic3.c_str())) {
            Serial.println("✅ Subscribed RPC: " + rpcTopic3);
        } else {
            Serial.println("⚠️ RPC Subscribe failed: " + rpcTopic3);
        }
        
        // 🔍 DEBUG: Subscribe wildcard to see ALL messages
        if (client.subscribe(rpcTopic4.c_str())) {
            Serial.println("✅ Subscribed WILDCARD (DEBUG): " + rpcTopic4);
        } else {
            Serial.println("⚠️ Wildcard Subscribe failed: " + rpcTopic4);
        }
        
        Serial.println("========================================\n");
        return true;
    }

    // ✅ Connection failed
    int rc = client.state();
    Serial.printf("❌ MQTT failed rc=%d\n", rc);
    Serial.println("\n📋 Error codes:");
    Serial.println("   rc=-4: Timeout");
    Serial.println("   rc=-3: Connection lost");
    Serial.println("   rc=-2: Connection failed");
    Serial.println("   rc=1: Wrong protocol");
    Serial.println("   rc=2: Client ID rejected");
    Serial.println("   rc=3: Server unavailable");
    Serial.println("   rc=4: Bad username/password");
    Serial.println("   rc=5: Not authorized");
    Serial.println("\n💡 Check:");
    Serial.println("   1. Client ID, Username, Password correct?");
    Serial.println("   2. Device activated on CoreIOT?");
    Serial.println("   3. Server & Port correct?");
    Serial.println("========================================\n");
    
    return false;
}

void coreiot_loop() {
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (mqttReconnect()) {
                lastReconnectAttempt = 0;
            }
        }
        return;
    }

#ifdef DEBUG
    Serial.println("STARTED PUBLISHING");
#endif

    client.loop();
}

void publishData(String json) {
    if (!client.connected()) {
        Serial.println("⚠️ MQTT not connected");
        return;
    }
    
    if (topicTelemetry.length() == 0) {
        topicTelemetry = coreiot_username + "/telemetry";
    }
    
    if (client.publish(topicTelemetry.c_str(), json.c_str(), false)) {
        Serial.println("✅ Published: " + json);
    } else {
        Serial.println("❌ Publish failed");
    }
}

bool isMQTTConnected() {
    return client.connected();
}

void CORE_IOT_reconnect() {
    if (!isMQTTConnected()) {
        mqttReconnect();
    }
}

// ✅ RPC - Điều khiển LED từ CoreIOT
void setLEDFromRPC(int ledNum, bool state, int brightness) {
    Serial.printf("\n🔧 RPC Control: LED%d = %s @ %d%%\n", 
                  ledNum, state ? "ON" : "OFF", brightness);
    setLED(ledNum, state, brightness);
}

bool getLEDStateFromRPC(int ledNum) {
    // Lấy trạng thái LED từ biến toàn cục trong mainserver.cpp
    if (ledNum == 1) {
        return led1.isOn;
    } else if (ledNum == 2) {
        return led2.isOn;
    }
    return false;
}
