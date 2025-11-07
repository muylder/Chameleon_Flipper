#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <furi.h>

#define RESPONSE_QUEUE_SIZE 8
#define RESPONSE_TIMEOUT_MS 2000

// Response structure
typedef struct {
    uint16_t cmd;
    uint16_t status;
    uint8_t data[512];
    uint16_t data_len;
    uint32_t timestamp;
    bool valid;
} ChameleonResponse;

// Response queue
typedef struct {
    ChameleonResponse responses[RESPONSE_QUEUE_SIZE];
    uint8_t read_idx;
    uint8_t write_idx;
    uint8_t count;
    FuriMutex* mutex;
} ChameleonResponseQueue;

// Response handler instance
typedef struct ChameleonResponseHandler ChameleonResponseHandler;

// Response callback - chamado quando resposta chega
typedef void (*ResponseReadyCallback)(ChameleonResponse* response, void* context);

// Create/destroy
ChameleonResponseHandler* chameleon_response_handler_alloc();
void chameleon_response_handler_free(ChameleonResponseHandler* handler);

// Set callback para quando resposta chega
void chameleon_response_handler_set_callback(
    ChameleonResponseHandler* handler,
    ResponseReadyCallback callback,
    void* context);

// Process incoming data (called by UART handler)
void chameleon_response_handler_process_data(
    ChameleonResponseHandler* handler,
    const uint8_t* data,
    size_t length);

// Wait for response to specific command
bool chameleon_response_handler_wait_for_response(
    ChameleonResponseHandler* handler,
    uint16_t cmd,
    ChameleonResponse* response,
    uint32_t timeout_ms);

// Get response if available (non-blocking)
bool chameleon_response_handler_get_response(
    ChameleonResponseHandler* handler,
    uint16_t cmd,
    ChameleonResponse* response);

// Clear all pending responses
void chameleon_response_handler_clear(ChameleonResponseHandler* handler);
