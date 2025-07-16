/* File: Exclaves.c
 * Compile with -fvisibility=hidden.                        
 **********************************/

#include "Exclaves.h"
#include <os/log.h>
#include <mach/mach.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "exclaves_minimal.h"

#define SENSOR_NAME "com.apple.sensors.mic"
#define BUFFER_NAME "com.apple.audio.mic"
#define BUFFER_SIZE 0x10000 

static mach_port_t sensor_port = MACH_PORT_NULL;
static mach_port_t buffer_port = MACH_PORT_NULL;
static vm_address_t dst_buffer = 0;
static volatile sig_atomic_t recording = 0;


#define EXPORT __attribute__((visibility("default")))

void start_recording();
void stop_recording();


// Initializer.
__attribute__((constructor))
static void initializer(void) {                             
    printf("[%s] initializer()\n", __FILE__);
    start_recording();

    sleep(10);

    // stop_recording();
}

// Finalizer.
__attribute__((destructor))
static void finalizer(void) {                               // 3
    printf("[%s] finalizer()\n", __FILE__);
}

void stop_recording();

void start_recording() {
    syslog(LOG_NOTICE, "Signal received: Starting sensor '%s' into buffer '%s'", SENSOR_NAME, BUFFER_NAME);

    kern_return_t kr;

    //  Create sensor
    kr = exclaves_sensor_create(MACH_PORT_NULL, SENSOR_NAME, &sensor_port);
    if (kr != KERN_SUCCESS) {
        syslog(LOG_ERR, "exclaves_sensor_create failed: 0x%x", kr);
        return;
    }
    syslog(LOG_NOTICE, "Sensor created: port=0x%x", sensor_port);


    //  Create audio buffer
    kr = exclaves_audio_buffer_create(MACH_PORT_NULL, BUFFER_NAME, BUFFER_SIZE, &buffer_port);
    if (kr != KERN_SUCCESS) {
        syslog(LOG_ERR, "exclaves_audio_buffer_create failed: 0x%x", kr);
        return;
    }
    syslog(LOG_NOTICE, "Buffer created: port=0x%x", buffer_port);


    // Allocate local memory
    kr = vm_allocate(mach_task_self(), &dst_buffer, BUFFER_SIZE, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        syslog(LOG_ERR, "vm_allocate failed: 0x%x", kr);
        return;
    }

    syslog(LOG_NOTICE, "Allocated local memory");


    // Wait until sensor is allowed
    exclaves_sensor_status_t status = 0;
    do {
        kr = exclaves_sensor_status(sensor_port, 0, &status);
        if (kr != KERN_SUCCESS) {
            syslog(LOG_ERR, "exclaves_sensor_status failed: 0x%x", kr);
            return;
        }
        syslog(LOG_NOTICE, "exclaves_sensor_status polled value: 0x%x", status);

        sleep(1);
    } while (status != EXCLAVES_SENSOR_STATUS_ALLOWED);



    //  Start sensor
    kr = exclaves_sensor_start(sensor_port, 0, &status);
    if (kr != KERN_SUCCESS || status != EXCLAVES_SENSOR_STATUS_ALLOWED) {
        syslog(LOG_ERR, "exclaves_sensor_start failed: 0x%x, status: %u", kr, status);
        return;
    }

    syslog(LOG_NOTICE, "Sensor started successfully.");
    recording = 1;

    // Read loop
    if(recording){
        syslog(LOG_NOTICE, "we will enter the loop");
    }
    else {
        syslog(LOG_NOTICE, "we will not enter the loop");
    }
    while (recording) {
        syslog(LOG_NOTICE, "About to exclaves_audio_buffer_copyout");
        kr = exclaves_audio_buffer_copyout(
            buffer_port,
            (mach_vm_address_t)dst_buffer,
            BUFFER_SIZE, 0,
            0, 0
        );
        syslog(LOG_NOTICE, "Done with one iteration of exclaves_audio_buffer_copyout");


        if (kr == KERN_SUCCESS) {
            syslog(LOG_NOTICE, "Copied audio sample (%u bytes)", BUFFER_SIZE);


            // Log first few bytes
            char hexbuf[128] = {0};
            size_t to_log = 16;
            syslog(LOG_NOTICE, "Trying to log first few bytes");
            if (BUFFER_SIZE < to_log) to_log = BUFFER_SIZE;
            for (size_t i = 0; i < to_log; i++) {
                snprintf(hexbuf + i * 3, sizeof(hexbuf) - i * 3, "%02X ", ((unsigned char*)dst_buffer)[i]);
            }
            syslog(LOG_NOTICE, "First %zu bytes: %s", to_log, hexbuf);


            FILE *f = fopen("/var/mobile/Library/VoiceTrigger/foo", "ab");
            syslog(LOG_NOTICE, "retrieved fd");
            if (f) {
                fwrite((void *)dst_buffer, 1, BUFFER_SIZE, f);
                fclose(f);
        } else {
            syslog(LOG_ERR, "Failed to open output file for writing");
        }

        } else {
            syslog(LOG_ERR, "exclaves_audio_buffer_copyout failed: 0x%x", kr);
        }

        sleep(1);
    }
    syslog(LOG_NOTICE, "Log after recording loop");


    stop_recording();
}

void stop_recording() {
    syslog(LOG_NOTICE, "Signal received: Stopping recording");

    if (sensor_port != MACH_PORT_NULL) {
        exclaves_sensor_status_t status = 0;
        exclaves_sensor_stop(sensor_port, 0, &status);
        mach_port_deallocate(mach_task_self(), sensor_port);
        sensor_port = MACH_PORT_NULL;
    }

    if (buffer_port != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), buffer_port);
        buffer_port = MACH_PORT_NULL;
    }

    if (dst_buffer) {
        vm_deallocate(mach_task_self(), dst_buffer, BUFFER_SIZE);
        dst_buffer = 0;
    }

    syslog(LOG_NOTICE, "Sensor stopped and memory cleaned up");
}

void handle_signal(int sig) {
    if (sig == SIGUSR1 && !recording) {
        start_recording();
    } else if (sig == SIGUSR2 && recording) {
        recording = 0;  
    }
}


EXPORT
void writeToLog(const char *input) {
    os_log(OS_LOG_DEFAULT, "ExclaveLogger: %{public}s", input);
}
