#pragma once

#include "services/uds_service_framework.h"

/** Handles authenticated ECU hard reset requests. */
UDS_NRC_t x11_EcuReset(void *context, const void *request, void *response, void *user);

/** Reports the diagnostic session, chip serial, and boot software version. */
UDS_NRC_t x22_ReadDataByIdentifier(void *context, const void *request,
                                    void *response, void *user);

/** Exchanges the versioned seed and checks the access key response. */
UDS_NRC_t x27_SecurityAccess(void *context, const void *request,
                             void *response, void *user);

/** Erases one authenticated application flash block. */
UDS_NRC_t x31_RoutineControl(void *context, const void *request,
                             void *response, void *user);

/** Starts an authenticated download within erased application flash. */
UDS_NRC_t x34_RequestDownload(void *context, const void *request,
                              void *response, void *user);

/** Writes the next aligned application data block. */
UDS_NRC_t x36_TransferData(void *context, const void *request,
                           void *response, void *user);

/** Completes a download after the declared byte count is written. */
UDS_NRC_t x37_RequestTransferExit(void *context, const void *request,
                                  void *response, void *user);
