//Direktori di .pio\libdeps\esp32dev\FirebaseClient\src
//Buat File Baru dengan nama "UserConfig.h"


#define ENABLE_DATABASE // For RTDB compilation
#define DISABLE_FIRESTORE // For Firestore compilation
#define DISABLE_FIRESTORE_QUERY // For Firestore Query feature compilation
#define DISABLE_MESSAGING // For Firebase Cloud Messaging compilation
#define DISABLE_STORAGE // For Firebase Storage compilation
#define DISABLE_CLOUD_STORAGE // For Google Cloud Storage compilation
#define DISABLE_FUNCTIONS // For Google Cloud Functions compilation
#define DISABLE_PSRAM // For enabling PSRAM support
#define DISABLE_OTA // For enabling OTA updates support via RTDB, Firebase Storage and Google Cloud Storage buckets
#define DISABLE_FS // For enabling Flash filesystem support

// For enabling authentication and token
#define ENABLE_SERVICE_AUTH
#define ENABLE_CUSTOM_AUTH
#define ENABLE_USER_AUTH
#define ENABLE_ACCESS_TOKEN
#define ENABLE_CUSTOM_TOKEN
#define ENABLE_ID_TOKEN
#define ENABLE_LEGACY_TOKEN

// For enabling non-sdk networking
#define DISABLE_ETHERNET_NETWORK
#define DISABLE_GSM_NETWORK