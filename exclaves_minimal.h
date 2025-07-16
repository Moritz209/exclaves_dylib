#ifndef EXCLAVES_MINIMAL_H
#define EXCLAVES_MINIMAL_H

#include <mach/mach.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------
// Sensor status enum — non-bitfield
// ----------------------------------------------------
typedef enum __attribute__((enum_extensibility(open))) exclaves_sensor_status : uint32_t {
    EXCLAVES_SENSOR_STATUS_ALLOWED = 1,
    EXCLAVES_SENSOR_STATUS_DENIED = 2,
    EXCLAVES_SENSOR_STATUS_CONTROL = 3
} exclaves_sensor_status_t;

// ----------------------------------------------------
// Sensor management APIs
// ----------------------------------------------------

kern_return_t exclaves_sensor_create(
    mach_port_t port,                    
    const char *sensor_name,            
    mach_port_t *out_sensor_port        
);

kern_return_t exclaves_sensor_status(
    mach_port_t sensor_port,
    uint64_t flags,
    exclaves_sensor_status_t *out_status
);

kern_return_t exclaves_sensor_start(
    mach_port_t sensor_port,
    uint64_t flags,
    exclaves_sensor_status_t *out_status
);

kern_return_t exclaves_sensor_stop(
    mach_port_t sensor_port,
    uint64_t flags,
    exclaves_sensor_status_t *out_status
);

// ----------------------------------------------------
// Audio buffer APIs
// ----------------------------------------------------

kern_return_t exclaves_audio_buffer_create(
    mach_port_t port,
    const char *buffer_name,
    mach_vm_size_t size,
    mach_port_t *out_audio_buffer_port
);

// Basic copyout: reads audio data into userspace
kern_return_t exclaves_audio_buffer_copyout(
    mach_port_t buffer_port,
    mach_vm_address_t dst_buffer,
    mach_vm_size_t size1,
    mach_vm_size_t offset1,
    mach_vm_size_t size2,
    mach_vm_size_t offset2
);

#ifdef __cplusplus
}
#endif

#endif // EXCLAVES_MINIMAL_H
