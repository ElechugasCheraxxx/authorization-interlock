#ifndef	_interlock_ipc_user_
#define	_interlock_ipc_user_

/* Module interlock_ipc */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#include "interlock_shared_types.h"

/* BEGIN VOUCHER CODE */

#ifndef KERNEL
#if defined(__has_include)
#if __has_include(<mach/mig_voucher_support.h>)
#ifndef USING_VOUCHERS
#define USING_VOUCHERS
#endif
#ifndef __VOUCHER_FORWARD_TYPE_DECLS__
#define __VOUCHER_FORWARD_TYPE_DECLS__
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __VOUCHER_FOWARD_TYPE_DECLS_SINGLE_ATTR
#define __VOUCHER_FOWARD_TYPE_DECLS_SINGLE_ATTR __unsafe_indexable
#endif
	extern boolean_t voucher_mach_msg_set(mach_msg_header_t * msg) __attribute__((weak_import));
#ifdef __cplusplus
}
#endif
#endif // __VOUCHER_FORWARD_TYPE_DECLS__
#endif // __has_include(<mach/mach_voucher_types.h>)
#endif // __has_include
#endif // !KERNEL
	
/* END VOUCHER CODE */

	
/* BEGIN MIG_STRNCPY_ZEROFILL CODE */

#if defined(__has_include)
#if __has_include(<mach/mig_strncpy_zerofill_support.h>)
#ifndef USING_MIG_STRNCPY_ZEROFILL
#define USING_MIG_STRNCPY_ZEROFILL
#endif
#ifndef __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__
#define __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS_CSTRING_ATTR
#define __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS_CSTRING_COUNTEDBY_ATTR(C) __unsafe_indexable
#endif
	extern int mig_strncpy_zerofill(char * dest, const char * src, int len) __attribute__((weak_import));
#ifdef __cplusplus
}
#endif
#endif /* __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__ */
#endif /* __has_include(<mach/mig_strncpy_zerofill_support.h>) */
#endif /* __has_include */
	
/* END MIG_STRNCPY_ZEROFILL CODE */


#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            * name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry   *function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	interlock_ipc_MSG_COUNT
#define	interlock_ipc_MSG_COUNT	3
#endif	/* interlock_ipc_MSG_COUNT */

#include <Availability.h>
#include <mach/std_types.h>
#include <mach/mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>

#ifdef __BeforeMigUserHeader
__BeforeMigUserHeader
#endif /* __BeforeMigUserHeader */

#include <sys/cdefs.h>
__BEGIN_DECLS


/* Routine interlock_dispatch_tactical_node */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t interlock_dispatch_tactical_node
(
	mach_port_t server_port,
	int sender_id,
	int msg_type,
	int priority,
	tact_msg_payload_t artifact,
	mach_msg_type_number_t payloadCnt
);

/* Routine interlock_fetch_blackboard_state */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t interlock_fetch_blackboard_state
(
	mach_port_t server_port,
	tact_msg_payload_t state_data,
	mach_msg_type_number_t *state_dataCnt
);

/* SimpleRoutine interlock_signal_escalation */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t interlock_signal_escalation
(
	mach_port_t server_port,
	int escalation_lvl,
	int reason_code
);

__END_DECLS

/********************** Caution **************************/
/* The following data types should be used to calculate  */
/* maximum message sizes only. The actual message may be */
/* smaller, and the position of the arguments within the */
/* message layout may vary from what is presented here.  */
/* For example, if any of the arguments are variable-    */
/* sized, and less than the maximum is sent, the data    */
/* will be packed tight in the actual message to reduce  */
/* the presence of holes.                                */
/********************** Caution **************************/

/* typedefs for all requests */

#ifndef __Request__interlock_ipc_subsystem__defined
#define __Request__interlock_ipc_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int sender_id;
		int msg_type;
		int priority;
		mach_msg_type_number_t payloadCnt;
		char artifact[1024];
	} __Request__interlock_dispatch_tactical_node_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
	} __Request__interlock_fetch_blackboard_state_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		int escalation_lvl;
		int reason_code;
	} __Request__interlock_signal_escalation_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif
#endif /* !__Request__interlock_ipc_subsystem__defined */

/* union of all requests */

#ifndef __RequestUnion__interlock_ipc_subsystem__defined
#define __RequestUnion__interlock_ipc_subsystem__defined
union __RequestUnion__interlock_ipc_subsystem {
	__Request__interlock_dispatch_tactical_node_t Request_interlock_dispatch_tactical_node;
	__Request__interlock_fetch_blackboard_state_t Request_interlock_fetch_blackboard_state;
	__Request__interlock_signal_escalation_t Request_interlock_signal_escalation;
};
#endif /* !__RequestUnion__interlock_ipc_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__interlock_ipc_subsystem__defined
#define __Reply__interlock_ipc_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__interlock_dispatch_tactical_node_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		mach_msg_type_number_t state_dataCnt;
		char state_data[1024];
	} __Reply__interlock_fetch_blackboard_state_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif

#ifdef  __MigPackStructs
#pragma pack(push, 4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__interlock_signal_escalation_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack(pop)
#endif
#endif /* !__Reply__interlock_ipc_subsystem__defined */

/* union of all replies */

#ifndef __ReplyUnion__interlock_ipc_subsystem__defined
#define __ReplyUnion__interlock_ipc_subsystem__defined
union __ReplyUnion__interlock_ipc_subsystem {
	__Reply__interlock_dispatch_tactical_node_t Reply_interlock_dispatch_tactical_node;
	__Reply__interlock_fetch_blackboard_state_t Reply_interlock_fetch_blackboard_state;
	__Reply__interlock_signal_escalation_t Reply_interlock_signal_escalation;
};
#endif /* !__RequestUnion__interlock_ipc_subsystem__defined */

#ifndef subsystem_to_name_map_interlock_ipc
#define subsystem_to_name_map_interlock_ipc \
    { "interlock_dispatch_tactical_node", 5000 },\
    { "interlock_fetch_blackboard_state", 5001 },\
    { "interlock_signal_escalation", 5002 }
#endif

#ifdef __AfterMigUserHeader
__AfterMigUserHeader
#endif /* __AfterMigUserHeader */

#endif	 /* _interlock_ipc_user_ */
