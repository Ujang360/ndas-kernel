#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include <assert.h>
#include <stdint.h>

#define MAX_PACKET_SIZE 2048
#define MAX_CPUS        128

#ifndef __packed
    #define __packed __attribute__((packed))
#endif

#define min(x, y) ((x) < (y) ? (x) : (y))

static_assert(sizeof(int) == sizeof(int32_t), "Invalid architecture!");
static_assert(sizeof(long) == sizeof(int64_t), "Invalid architecture!");

typedef struct xdp_md XDPContext;

typedef struct __packed OffloadedPacket {
    __u16 length;
} OffloadedPacket;

char _license[] SEC("license") = "GPL";

struct bpf_map_def SEC("maps") my_map = {
    .type = BPF_MAP_TYPE_PERF_EVENT_ARRAY,
    .key_size = sizeof(int32_t),
    .value_size = sizeof(__u32),
    .max_entries = MAX_CPUS,
};

SEC("ndas_dumper")
int32_t ndas_kernel(XDPContext* ctx) {
    void* data_end = (void*)(int64_t)ctx->data_end;
    void* data = (void*)(int64_t)ctx->data;

    if (data < data_end) {
        __u64 flags = BPF_F_CURRENT_CPU;
        __u16 sample_size;
        OffloadedPacket packet;
        packet.length = (__u16)(data_end - data);
        sample_size = min(packet.length, MAX_PACKET_SIZE);
        flags |= ((__u64)sample_size) << 32;
        bpf_perf_event_output(ctx, &my_map, flags, &packet, sizeof(OffloadedPacket));
    }

    return XDP_PASS;
}
