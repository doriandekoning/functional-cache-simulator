#ifndef __CORHERENCY_PROTOCOL_H
#define __CORHERENCY_PROTOCOL_H

typedef int (*NewStateFunc)(int old_state, int event, int* bus_request);
typedef bool (*FlushNeededFunc)(int cur_state);

struct CoherencyProtocol{
	NewStateFunc new_state_func;
	FlushNeededFunc flush_needed_on_evict;
};

#endif
