#include "response_handler.h"
#include "../chameleon_protocol/chameleon_protocol.h"
#include <furi.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ResponseHandler"

struct ChameleonResponseHandler {
    ChameleonResponseQueue queue;
    ResponseReadyCallback callback;
    void* callback_context;

    // Buffer para acumular dados recebidos
    uint8_t rx_buffer[1024];
    size_t rx_buffer_len;

    ChameleonProtocol* protocol;
};

ChameleonResponseHandler* chameleon_response_handler_alloc() {
    ChameleonResponseHandler* handler = malloc(sizeof(ChameleonResponseHandler));
    memset(handler, 0, sizeof(ChameleonResponseHandler));

    handler->queue.mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    handler->protocol = chameleon_protocol_alloc();

    return handler;
}

void chameleon_response_handler_free(ChameleonResponseHandler* handler) {
    furi_assert(handler);

    furi_mutex_free(handler->queue.mutex);
    chameleon_protocol_free(handler->protocol);
    free(handler);
}

void chameleon_response_handler_set_callback(
    ChameleonResponseHandler* handler,
    ResponseReadyCallback callback,
    void* context) {
    furi_assert(handler);
    handler->callback = callback;
    handler->callback_context = context;
}

static bool queue_push(ChameleonResponseQueue* queue, ChameleonResponse* response) {
    furi_assert(queue);
    furi_assert(response);

    furi_mutex_acquire(queue->mutex, FuriWaitForever);

    if(queue->count >= RESPONSE_QUEUE_SIZE) {
        FURI_LOG_W(TAG, "Queue full, dropping oldest response");
        queue->read_idx = (queue->read_idx + 1) % RESPONSE_QUEUE_SIZE;
        queue->count--;
    }

    memcpy(&queue->responses[queue->write_idx], response, sizeof(ChameleonResponse));
    queue->write_idx = (queue->write_idx + 1) % RESPONSE_QUEUE_SIZE;
    queue->count++;

    furi_mutex_release(queue->mutex);

    return true;
}

static bool queue_find_and_pop(ChameleonResponseQueue* queue, uint16_t cmd, ChameleonResponse* response) {
    furi_assert(queue);
    furi_assert(response);

    furi_mutex_acquire(queue->mutex, FuriWaitForever);

    bool found = false;

    for(uint8_t i = 0; i < queue->count; i++) {
        uint8_t idx = (queue->read_idx + i) % RESPONSE_QUEUE_SIZE;

        if(queue->responses[idx].cmd == cmd && queue->responses[idx].valid) {
            // Found it!
            memcpy(response, &queue->responses[idx], sizeof(ChameleonResponse));

            // Remove from queue by shifting
            for(uint8_t j = i; j < queue->count - 1; j++) {
                uint8_t src_idx = (queue->read_idx + j + 1) % RESPONSE_QUEUE_SIZE;
                uint8_t dst_idx = (queue->read_idx + j) % RESPONSE_QUEUE_SIZE;
                memcpy(&queue->responses[dst_idx], &queue->responses[src_idx], sizeof(ChameleonResponse));
            }

            queue->count--;
            queue->write_idx = (queue->write_idx - 1 + RESPONSE_QUEUE_SIZE) % RESPONSE_QUEUE_SIZE;

            found = true;
            break;
        }
    }

    furi_mutex_release(queue->mutex);

    return found;
}

void chameleon_response_handler_process_data(
    ChameleonResponseHandler* handler,
    const uint8_t* data,
    size_t length) {
    furi_assert(handler);
    furi_assert(data);

    // Append to receive buffer
    if(handler->rx_buffer_len + length > sizeof(handler->rx_buffer)) {
        FURI_LOG_W(TAG, "RX buffer overflow, clearing");
        handler->rx_buffer_len = 0;
    }

    memcpy(&handler->rx_buffer[handler->rx_buffer_len], data, length);
    handler->rx_buffer_len += length;

    // Try to parse frames from buffer
    while(handler->rx_buffer_len >= CHAMELEON_FRAME_OVERHEAD) {
        // Check for SOF
        size_t sof_idx = 0;
        bool found_sof = false;

        for(size_t i = 0; i < handler->rx_buffer_len; i++) {
            if(handler->rx_buffer[i] == CHAMELEON_SOF) {
                sof_idx = i;
                found_sof = true;
                break;
            }
        }

        if(!found_sof) {
            // No SOF found, clear buffer
            handler->rx_buffer_len = 0;
            break;
        }

        // Move SOF to beginning if needed
        if(sof_idx > 0) {
            memmove(handler->rx_buffer, &handler->rx_buffer[sof_idx], handler->rx_buffer_len - sof_idx);
            handler->rx_buffer_len -= sof_idx;
        }

        // Need at least header to get frame length
        if(handler->rx_buffer_len < 9) {
            // Not enough data yet
            break;
        }

        // Get expected frame length
        size_t expected_len = chameleon_protocol_get_expected_frame_len(handler->rx_buffer);

        if(expected_len == 0) {
            // Invalid frame, skip SOF
            memmove(handler->rx_buffer, &handler->rx_buffer[1], handler->rx_buffer_len - 1);
            handler->rx_buffer_len--;
            continue;
        }

        if(handler->rx_buffer_len < expected_len) {
            // Not enough data yet, wait for more
            break;
        }

        // Try to parse the frame
        ChameleonResponse response = {0};
        response.timestamp = furi_get_tick();

        if(chameleon_protocol_parse_frame(
               handler->protocol,
               handler->rx_buffer,
               expected_len,
               &response.cmd,
               &response.status,
               response.data,
               &response.data_len)) {

            response.valid = true;

            FURI_LOG_I(
                TAG,
                "Received response: CMD=%04X, STATUS=%04X, LEN=%u",
                response.cmd,
                response.status,
                response.data_len);

            // Add to queue
            queue_push(&handler->queue, &response);

            // Call callback if set
            if(handler->callback) {
                handler->callback(&response, handler->callback_context);
            }
        } else {
            FURI_LOG_E(TAG, "Failed to parse frame");
        }

        // Remove processed frame from buffer
        memmove(handler->rx_buffer, &handler->rx_buffer[expected_len], handler->rx_buffer_len - expected_len);
        handler->rx_buffer_len -= expected_len;
    }
}

bool chameleon_response_handler_wait_for_response(
    ChameleonResponseHandler* handler,
    uint16_t cmd,
    ChameleonResponse* response,
    uint32_t timeout_ms) {
    furi_assert(handler);
    furi_assert(response);

    uint32_t start_time = furi_get_tick();

    while((furi_get_tick() - start_time) < timeout_ms) {
        if(queue_find_and_pop(&handler->queue, cmd, response)) {
            return true;
        }

        furi_delay_ms(10);
    }

    FURI_LOG_W(TAG, "Timeout waiting for response to CMD=%04X", cmd);
    return false;
}

bool chameleon_response_handler_get_response(
    ChameleonResponseHandler* handler,
    uint16_t cmd,
    ChameleonResponse* response) {
    furi_assert(handler);
    furi_assert(response);

    return queue_find_and_pop(&handler->queue, cmd, response);
}

void chameleon_response_handler_clear(ChameleonResponseHandler* handler) {
    furi_assert(handler);

    furi_mutex_acquire(handler->queue.mutex, FuriWaitForever);

    handler->queue.read_idx = 0;
    handler->queue.write_idx = 0;
    handler->queue.count = 0;
    handler->rx_buffer_len = 0;

    furi_mutex_release(handler->queue.mutex);

    FURI_LOG_I(TAG, "Response queue cleared");
}
