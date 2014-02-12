/******************************************************************************
 * Copyright (c) 2013 - 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <alljoyn.h>
#include <errno.h>
#include "PropertyStoreOEMProvisioning.h"
#include <alljoyn/services_common/PropertyStore.h>
#include <alljoyn/services_common/Services_Common.h>
#ifdef ONBOARDING_SERVICE
    #include <alljoyn/onboarding/OnboardingManager.h>
#endif
#include <aj_sasl.h>
#include <aj_guid.h>
#include <aj_nvram.h>
#include <aj_creds.h>
#include <aj_config.h>

const PropertyStoreEntry propertyStoreProperties[AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS] =
{
//  { "Key Name            ", W, A, M, I .. . . ., P },
    { "DeviceId",             0, 1, 0, 1, 0, 0, 0, 1 },
    { "AppId",                0, 1, 0, 1, 0, 0, 0, 1 },
    { "DeviceName",           1, 1, 0, 1, 0, 0, 0, 1 },
// Add other persisted keys above this line
    { "DefaultLanguage",      1, 1, 0, 0, 0, 0, 0, 1 },
    { "Passcode",             1, 0, 0, 0, 0, 0, 0, 0 },
    { "RealmName",            1, 0, 0, 0, 0, 0, 0, 0 },
// Add other configurable keys above this line
    { "AppName",              0, 1, 0, 0, 0, 0, 0, 1 },
    { "Description",          0, 0, 1, 0, 0, 0, 0, 1 },
    { "Manufacturer",         0, 1, 1, 0, 0, 0, 0, 1 },
    { "ModelNumber",          0, 1, 0, 0, 0, 0, 0, 1 },
    { "DateOfManufacture",    0, 0, 0, 0, 0, 0, 0, 1 },
    { "SoftwareVersion",      0, 0, 0, 0, 0, 0, 0, 1 },
    { "AJSoftwareVersion",    0, 0, 0, 0, 0, 0, 0, 1 },
    { "HardwareVersion",      0, 0, 0, 0, 0, 0, 0, 1 },
    { "SupportUrl",           0, 0, 1, 0, 0, 0, 0, 1 },
#if defined CONFIG_SERVICE
    { "MaxLength",            0, 0, 1, 0, 0, 0, 0, 1 },
#endif
// Add other about keys above this line
};

static const char* defaultLanguagesKeyName = { "SupportedLanguages" };

uint8_t AJSVC_PropertyStore_GetMaxValueLength(AJSVC_PropertyStoreFieldIndices fieldIndex)
{
    switch (fieldIndex) {
    case AJSVC_PROPERTY_STORE_DEVICE_NAME:
        return DEVICE_NAME_VALUE_LENGTH;

    case AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE:
        return LANG_VALUE_LENGTH;

    case AJSVC_PROPERTY_STORE_PASSCODE:
        return PASSWORD_VALUE_LENGTH;

    default:
        return KEY_VALUE_LENGTH;
    }
}

const char* AJSVC_PropertyStore_GetFieldNameForIndex(AJSVC_PropertyStoreFieldIndices fieldIndex)
{
    return propertyStoreProperties[fieldIndex].keyName;
}

AJSVC_PropertyStoreFieldIndices AJSVC_PropertyStore_GetIndexOfFieldName(const char* fieldName)
{
    AJSVC_PropertyStoreFieldIndices fieldIndex = 0;
    for (; fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; fieldIndex++) {
        if (!strcmp(propertyStoreProperties[fieldIndex].keyName, fieldName)) {
            return fieldIndex;
        }
    }
    return AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX;
}

static int8_t GetLanguageIndexForProperty(int8_t langIndex, AJSVC_PropertyStoreFieldIndices fieldIndex)
{
    if (propertyStoreProperties[fieldIndex].mode2MultiLng) {
        return langIndex;
    }
    return AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
}

const char* AJSVC_PropertyStore_GetValueForLang(AJSVC_PropertyStoreFieldIndices fieldIndex, int8_t langIndex)
{
    if ((int8_t)fieldIndex <= (int8_t)AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || (int8_t)fieldIndex >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS) {
        return NULL;
    }
    langIndex = GetLanguageIndexForProperty(langIndex, fieldIndex);
    if (langIndex <= AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX || langIndex >= AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES) {
        return NULL;
    }
    if (fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS &&
        (propertyStoreProperties[fieldIndex].mode0Write || propertyStoreProperties[fieldIndex].mode3Init) &&
        propertyStoreRuntimeValues[fieldIndex].value != NULL &&
        (propertyStoreRuntimeValues[fieldIndex].value[langIndex]) != NULL &&
        (propertyStoreRuntimeValues[fieldIndex].value[langIndex])[0] != '\0') {
        AJ_Printf("Has key [%s] value [%s]\n", propertyStoreProperties[fieldIndex].keyName, propertyStoreRuntimeValues[fieldIndex].value[langIndex]);
        return propertyStoreRuntimeValues[fieldIndex].value[langIndex];
    } else if (propertyStoreDefaultValues[fieldIndex] != NULL &&
               (propertyStoreDefaultValues[fieldIndex])[langIndex] != NULL) {
        AJ_Printf("Has key [%s] defaultValue [%s]\n", propertyStoreProperties[fieldIndex].keyName, (propertyStoreDefaultValues[fieldIndex])[langIndex]);
        return (propertyStoreDefaultValues[fieldIndex])[langIndex];
    }

    return NULL;
}

const char* AJSVC_PropertyStore_GetValue(AJSVC_PropertyStoreFieldIndices fieldIndex)
{
    return AJSVC_PropertyStore_GetValueForLang(fieldIndex, AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX);
}

int8_t AJSVC_PropertyStore_GetLanguageIndex(const char* const language)
{
    uint8_t langIndex;
    if (language != NULL) {
        langIndex = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
        for (; langIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES; langIndex++) {
            if (language[0] == '\0') { // Check for empty language, if yes then search for current default language index
#ifdef CONFIG_SERVICE
                if (propertyStoreRuntimeValues[AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE].value[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX]) {
                    if (!strcmp(propertyStoreRuntimeValues[AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE].value[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX], propertyStoreDefaultLanguages[langIndex])) {
                        return (int8_t)langIndex;
                    }
                } else {
#endif
                if (!strcmp((propertyStoreDefaultValues[AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE])[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX], propertyStoreDefaultLanguages[langIndex])) {
                    return (int8_t)langIndex;
                }
#ifdef CONFIG_SERVICE
            }
#endif
            } else {
                if (!strcmp(language, propertyStoreDefaultLanguages[langIndex])) {
                    return (int8_t)langIndex;
                }
            }
        }
    }
    return AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX;
}

uint8_t AJSVC_PropertyStore_SetValueForLang(AJSVC_PropertyStoreFieldIndices fieldIndex, int8_t langIndex, const char* value)
{
    size_t var_size;
    if ((int8_t)fieldIndex <= (int8_t)AJSVC_PROPERTY_STORE_ERROR_FIELD_INDEX || (int8_t)fieldIndex >= (int8_t)AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS) {
        return FALSE;
    }
    langIndex = GetLanguageIndexForProperty(langIndex, fieldIndex);
    if (langIndex <= AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX || langIndex >= AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES) {
        return FALSE;
    }
    AJ_Printf("Set key [%s] defaultValue [%s]\n", propertyStoreProperties[fieldIndex].keyName, value);
    var_size = propertyStoreRuntimeValues[fieldIndex].size;
    strncpy(propertyStoreRuntimeValues[fieldIndex].value[langIndex], value, var_size - 1);
    (propertyStoreRuntimeValues[fieldIndex].value[langIndex])[var_size - 1] = '\0';

    return TRUE;
}

uint8_t AJSVC_PropertyStore_SetValue(AJSVC_PropertyStoreFieldIndices fieldIndex, const char* value)
{
    return AJSVC_PropertyStore_SetValueForLang(fieldIndex, AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX, value);
}

int8_t AJSVC_PropertyStore_GetCurrentDefaultLanguageIndex()
{
    const char* currentDefaultLanguage = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEFAULT_LANGUAGE);
    int8_t currentDefaultLanguageIndex = AJSVC_PropertyStore_GetLanguageIndex(currentDefaultLanguage);
    if (currentDefaultLanguageIndex == AJSVC_PROPERTY_STORE_ERROR_LANGUAGE_INDEX) {
        currentDefaultLanguageIndex = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
        AJ_Printf("Failed to find default language %s defaulting to %s", (currentDefaultLanguage != NULL ? currentDefaultLanguage : "NULL"), propertyStoreDefaultLanguages[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX]);
    }
    return currentDefaultLanguageIndex;
}

#ifdef CONFIG_SERVICE
static void ClearPropertiesInRAM()
{
    uint8_t langIndex;
    char* buf;
    AJSVC_PropertyStoreFieldIndices fieldIndex = 0;
    for (; fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS; fieldIndex++) {
        if (propertyStoreRuntimeValues[fieldIndex].value) {
            langIndex = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
            for (; langIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES; langIndex++) {
                if (propertyStoreProperties[fieldIndex].mode2MultiLng || langIndex == AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX) {
                    buf = propertyStoreRuntimeValues[fieldIndex].value[langIndex];
                    if (buf) {
                        memset(buf, 0, propertyStoreRuntimeValues[fieldIndex].size);
                    }
                }
            }
        }
    }
}
#endif

static void InitMandatoryPropertiesInRAM()
{
    char* machineIdValue = propertyStoreRuntimeValues[AJSVC_PROPERTY_STORE_APP_ID].value[AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX];
    const char* currentAppIdValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_APP_ID);
    const char* currentDeviceIdValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEVICE_ID);
    const char* currentDeviceNameValue = AJSVC_PropertyStore_GetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME);
    size_t serialIdLen = 0;
    size_t machineIdLen = 0;
    AJ_GUID theAJ_GUID;
    AJ_Status status;
    char deviceName[DEVICE_NAME_VALUE_LENGTH + 1] = { 0 };
    if (currentAppIdValue == NULL || currentAppIdValue[0] == '\0') {
        status = AJ_GetLocalGUID(&theAJ_GUID);
        if (status == AJ_OK) {
            AJ_GUID_ToString(&theAJ_GUID, machineIdValue, propertyStoreRuntimeValues[AJSVC_PROPERTY_STORE_APP_ID].size);
        }
    }
    if (currentDeviceIdValue == NULL || currentDeviceIdValue[0] == '\0') {
        AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_ID, machineIdValue);
    }
    if (currentDeviceNameValue == NULL || currentDeviceNameValue[0] == '\0') {
#ifdef ONBOARDING_SERVICE
        serialIdLen = AJOBS_DEVICE_SERIAL_ID_LEN;
#else
        serialIdLen = 7;
#endif
        machineIdLen = strlen(machineIdValue);
#ifdef _WIN32
        _snprintf(deviceName, DEVICE_NAME_VALUE_LENGTH + 1, "%s %s %s", deviceManufactureName, deviceProductName, &machineIdValue[machineIdLen - min(serialIdLen, machineIdLen)]);
#else
        snprintf(deviceName, DEVICE_NAME_VALUE_LENGTH + 1, "%s %s %s", deviceManufactureName, deviceProductName, &machineIdValue[machineIdLen - min(serialIdLen, machineIdLen)]);
#endif
        AJSVC_PropertyStore_SetValue(AJSVC_PROPERTY_STORE_DEVICE_NAME, deviceName);
    }
}

AJ_Status PropertyStore_Init()
{
    AJ_Status status = AJ_OK;
#ifdef CONFIG_SERVICE
    status = AJSVC_PropertyStore_LoadAll();
#endif
    InitMandatoryPropertiesInRAM();
    return status;
}

#ifdef CONFIG_SERVICE
static AJ_Status PropertyStore_ReadConfig(uint16_t index, void* ptr, size_t size)
{
    AJ_Status status = AJ_OK;
    size_t sizeRead = 0;

    AJ_NV_DATASET* nvramHandle = AJ_NVRAM_Open(index, "r", 0);
    if (nvramHandle != NULL) {
        sizeRead = AJ_NVRAM_Read(ptr, size, nvramHandle);
        status = AJ_NVRAM_Close(nvramHandle);
        if (sizeRead != sizeRead) {
            status = AJ_ERR_WRITE;
        }
    }

    return status;
}

static AJ_Status PropertyStore_WriteConfig(uint16_t index, void* ptr, size_t size, char* mode)
{
    AJ_Status status = AJ_OK;
    size_t sizeWritten = 0;

    AJ_NV_DATASET* nvramHandle = AJ_NVRAM_Open(index, mode, size);
    if (nvramHandle != NULL) {
        sizeWritten = AJ_NVRAM_Write(ptr, size, nvramHandle);
        status = AJ_NVRAM_Close(nvramHandle);
        if (sizeWritten != size) {
            status = AJ_ERR_WRITE;
        }
    }

    return status;
}

AJ_Status AJSVC_PropertyStore_LoadAll()
{
    AJ_Status status = AJ_OK;
    void* buf = NULL;
    size_t size = 0;
    uint16_t entry;

    int8_t langIndex = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
    for (; langIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES; langIndex++) {
        AJSVC_PropertyStoreFieldIndices fieldIndex = 0;
        for (; fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS; fieldIndex++) {
            if (propertyStoreRuntimeValues[fieldIndex].value == NULL ||
                (langIndex != AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX && !propertyStoreProperties[fieldIndex].mode2MultiLng)) {
                continue;
            }
            buf = propertyStoreRuntimeValues[fieldIndex].value[langIndex];
            if (buf) {
                size = propertyStoreRuntimeValues[fieldIndex].size;
                entry = (int)fieldIndex + (int)langIndex * (int)AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS;
                status = PropertyStore_ReadConfig(AJ_PROPERTIES_NV_ID_BEGIN + entry, buf, size);
                AJ_Printf("nvram read fieldIndex=%d langIndex=%d entry=%d val=%s size=%lu status=%s\n", (int)fieldIndex, (int)langIndex, (int)entry, propertyStoreRuntimeValues[fieldIndex].value[langIndex], size, AJ_StatusText(status));
            }
        }
    }

    return status;
}

AJ_Status AJSVC_PropertyStore_SaveAll()
{
    AJ_Status status = AJ_OK;
    void* buf = NULL;
    size_t size = 0;
    uint16_t entry;

    int8_t langIndex = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
    for (; langIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES; langIndex++) {
        AJSVC_PropertyStoreFieldIndices fieldIndex = 0;
        for (; fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS; fieldIndex++) {
            if (propertyStoreRuntimeValues[fieldIndex].value == NULL ||
                (langIndex != AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX && !propertyStoreProperties[fieldIndex].mode2MultiLng)) {
                continue;
            }
            buf = propertyStoreRuntimeValues[fieldIndex].value[langIndex];
            if (buf) {
                size = propertyStoreRuntimeValues[fieldIndex].size;
                entry = (int)fieldIndex + (int)langIndex * (int)AJSVC_PROPERTY_STORE_NUMBER_OF_CONFIG_KEYS;
                status = PropertyStore_WriteConfig(AJ_PROPERTIES_NV_ID_BEGIN + entry, buf, size, "w");
                AJ_Printf("nvram write fieldIndex=%d langIndex=%d entry=%d val=%s size=%lu status=%s\n", (int)fieldIndex, (int)langIndex, (int)entry, propertyStoreRuntimeValues[fieldIndex].value[langIndex], size, AJ_StatusText(status));
            }
        }
    }
    AJ_About_SetShouldAnnounce(TRUE); // Set flag for sending an updated Announcement

    return status;
}

static uint8_t UpdateFieldInRAM(AJSVC_PropertyStoreFieldIndices fieldIndex, int8_t langIndex, const char* fieldValue)
{
    uint8_t ret = FALSE;

    if (propertyStoreProperties[fieldIndex].mode0Write && propertyStoreProperties[fieldIndex].mode7Public) {
        ret = AJSVC_PropertyStore_SetValueForLang(fieldIndex, langIndex, fieldValue);
    } else {
        AJ_Printf("UpdateFieldInRAM ERROR - field %s has read only attribute or is private\n", propertyStoreProperties[fieldIndex].keyName);
    }

    return ret;
}

static uint8_t DeleteFieldFromRAM(AJSVC_PropertyStoreFieldIndices fieldIndex, int8_t langIndex)
{
    return UpdateFieldInRAM(fieldIndex, langIndex, "");
}
#endif

AJ_Status AJSVC_PropertyStore_ReadAll(AJ_Message* msg, AJSVC_PropertyStoreCategoryFilter filter, int8_t langIndex)
{
    AJ_Status status = AJ_OK;
    AJ_Arg array;
    AJ_Arg array2;
    AJ_Arg dict;
    const char* value;
    AJ_Arg arg;
    uint8_t rawValue[16];
    uint8_t index;

    AJ_Printf("PropertyStore_ReadAll()\n");

    status = AJ_MarshalContainer(msg, &array, AJ_ARG_ARRAY);
    if (status != AJ_OK) {
        return status;
    }

    AJSVC_PropertyStoreFieldIndices fieldIndex = 0;
    for (; fieldIndex < AJSVC_PROPERTY_STORE_NUMBER_OF_KEYS; fieldIndex++) {
#ifdef CONFIG_SERVICE
        if (propertyStoreProperties[fieldIndex].mode7Public && (filter.bit0About || (filter.bit1Config && propertyStoreProperties[fieldIndex].mode0Write) || (filter.bit2Announce && propertyStoreProperties[fieldIndex].mode1Announce))) {
#else
        if (propertyStoreProperties[fieldIndex].mode7Public && (filter.bit0About || (filter.bit2Announce && propertyStoreProperties[fieldIndex].mode1Announce))) {
#endif
            value = AJSVC_PropertyStore_GetValueForLang(fieldIndex, langIndex);

            if (value == NULL) {     // Non existing values are skipped!
                AJ_Printf("PropertyStore_ReadAll - Failed to get value for fieldIndex=%d langIndex=%d, skipping.\n", (int)fieldIndex, (int)langIndex);
            } else {
                status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
                if (status != AJ_OK) {
                    return status;
                }
                status = AJ_MarshalArgs(msg, "s", propertyStoreProperties[fieldIndex].keyName);
                if (status != AJ_OK) {
                    return status;
                }

                if (fieldIndex == AJSVC_PROPERTY_STORE_APP_ID) {
                    status = AJ_MarshalVariant(msg, "ay");
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_HexToRaw(value, 0, rawValue, (size_t)sizeof(rawValue));
                    status = AJ_MarshalArg(msg, AJ_InitArg(&arg, AJ_ARG_BYTE, AJ_ARRAY_FLAG, rawValue, sizeof(rawValue)));
                    if (status != AJ_OK) {
                        return status;
                    }
#ifdef CONFIG_SERVICE
                } else if (fieldIndex == AJSVC_PROPERTY_STORE_MAX_LENGTH) {
                    status = AJ_MarshalVariant(msg, "q");
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_MarshalArgs(msg, "q", DEVICE_NAME_VALUE_LENGTH);
                    if (status != AJ_OK) {
                        return status;
                    }
#endif
                } else if (fieldIndex == AJSVC_PROPERTY_STORE_AJ_SOFTWARE_VERSION) {
                    status = AJ_MarshalVariant(msg, "s");
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_MarshalArgs(msg, "s", AJ_GetVersion());
                    if (status != AJ_OK) {
                        return status;
                    }
                } else {
                    status = AJ_MarshalVariant(msg, "s");
                    if (status != AJ_OK) {
                        return status;
                    }
                    status = AJ_MarshalArgs(msg, "s", value);
                    if (status != AJ_OK) {
                        return status;
                    }
                }
                status = AJ_MarshalCloseContainer(msg, &dict);
                if (status != AJ_OK) {
                    return status;
                }
            }
        }
    }

    if (filter.bit0About) {
        // Add supported languages
        status = AJ_MarshalContainer(msg, &dict, AJ_ARG_DICT_ENTRY);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalArgs(msg, "s", defaultLanguagesKeyName);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalVariant(msg, "as");
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalContainer(msg, &array2, AJ_ARG_ARRAY);
        if (status != AJ_OK) {
            return status;
        }

        index = AJSVC_PROPERTY_STORE_NO_LANGUAGE_INDEX;
        for (; index < AJSVC_PROPERTY_STORE_NUMBER_OF_LANGUAGES; index++) {
            status = AJ_MarshalArgs(msg, "s", propertyStoreDefaultLanguages[index]);
            if (status != AJ_OK) {
                return status;
            }
        }

        status = AJ_MarshalCloseContainer(msg, &array2);
        if (status != AJ_OK) {
            return status;
        }
        status = AJ_MarshalCloseContainer(msg, &dict);
        if (status != AJ_OK) {
            return status;
        }
    }
    status = AJ_MarshalCloseContainer(msg, &array);
    if (status != AJ_OK) {
        return status;
    }

    return status;
}

#ifdef CONFIG_SERVICE
AJ_Status AJSVC_PropertyStore_Update(const char* key, int8_t langIndex, const char* value)
{
    if (UpdateFieldInRAM(AJSVC_PropertyStore_GetIndexOfFieldName(key), langIndex, value)) {
        return AJ_OK;
    }

    return AJ_ERR_FAILURE;
}

AJ_Status AJSVC_PropertyStore_Reset(const char* key, int8_t langIndex)
{
    if (DeleteFieldFromRAM(AJSVC_PropertyStore_GetIndexOfFieldName(key), langIndex)) {
        InitMandatoryPropertiesInRAM();
        return AJ_OK;
    }

    return AJ_ERR_FAILURE;
}

AJ_Status AJSVC_PropertyStore_ResetAll()
{
    ClearPropertiesInRAM();
    InitMandatoryPropertiesInRAM();
    return AJSVC_PropertyStore_SaveAll();
}
#endif
