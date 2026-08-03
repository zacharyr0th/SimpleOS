#include "ipc/pipe.h"
#include "mm/kmalloc.h"
#include "kernel/scheduler.h"
#include "lib/string.h"
#include "kernel/process.h"

// Create a new pipe
pipe_t* pipe_create(void) {
    pipe_t* pipe = (pipe_t*)kmalloc(sizeof(pipe_t));
    if (!pipe) return NULL;

    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->count = 0;
    pipe->reader_closed = 0;
    pipe->writer_closed = 0;
    pipe->readers = 1;
    pipe->writers = 1;

    return pipe;
}

// Free a pipe outright, bypassing refcounts. Only valid before any fd
// references it (i.e. the sys_pipe creation-failure path).
void pipe_free(pipe_t* pipe) {
    if (pipe) kfree(pipe);
}

// Account a newly duplicated fd (fork/dup2) against the pipe.
void pipe_add_ref(pipe_t* pipe, int writer) {
    if (!pipe) return;
    if (writer) pipe->writers++;
    else pipe->readers++;
}

// Release one fd's reference. The last reader marks EOF for writers; the
// last writer marks EOF for readers; the last reference frees the pipe.
void pipe_close_ref(pipe_t* pipe, int writer) {
    if (!pipe) return;
    if (writer) {
        if (pipe->writers > 0 && --pipe->writers == 0) {
            pipe->writer_closed = 1;
        }
    } else {
        if (pipe->readers > 0 && --pipe->readers == 0) {
            pipe->reader_closed = 1;
        }
    }
    if (pipe->readers == 0 && pipe->writers == 0) {
        // Both ends gone: nobody can be blocked on it, so free it.
        kfree(pipe);
        return;
    }
    // One end closed while the other stays open: wake a blocked peer so it can
    // observe EOF (blocked reader) or broken pipe (blocked writer).
    process_wake_channel(pipe);
}

int pipe_read(pipe_t* pipe, void* buffer, size_t count) {
    if (!pipe || pipe->reader_closed) return -1;

    uint8_t* buf = (uint8_t*)buffer;
    size_t bytes_read = 0;

    while (bytes_read < count) {
        if (pipe->count == 0) {
            // Drained: EOF once the writer is gone, otherwise block for data.
            if (pipe->writer_closed) {
                break;
            }
            process_wake_channel(pipe);  // release any writer blocked on a full pipe
            process_block_on(pipe);      // sleep until a writer signals
            continue;
        }

        buf[bytes_read++] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_SIZE;
        pipe->count--;
    }

    if (bytes_read > 0) {
        process_wake_channel(pipe);  // freed space: wake blocked writers
    }
    return bytes_read;
}

int pipe_write(pipe_t* pipe, const void* buffer, size_t count) {
    if (!pipe || pipe->writer_closed) return -1;

    // If reader closed, return error (broken pipe)
    if (pipe->reader_closed) return -1;

    const uint8_t* buf = (const uint8_t*)buffer;
    size_t bytes_written = 0;

    while (bytes_written < count) {
        // Reader went away mid-write: stop (broken pipe).
        if (pipe->reader_closed) {
            break;
        }
        if (pipe->count == PIPE_SIZE) {
            process_wake_channel(pipe);  // let readers drain what we've written
            process_block_on(pipe);      // sleep until space frees up
            continue;
        }

        pipe->buffer[pipe->write_pos] = buf[bytes_written++];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_SIZE;
        pipe->count++;
    }

    if (bytes_written > 0) {
        process_wake_channel(pipe);  // new data: wake blocked readers
    }
    return bytes_written;
}
