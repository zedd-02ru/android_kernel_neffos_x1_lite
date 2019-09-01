
#ifndef __TEE_H
#define __TEE_H

#include <linux/ioctl.h>
#include <linux/types.h>

/*
 * This file describes the API provided by a TEE driver to user space.
 *
 * Each TEE driver defines a TEE specific protocol which is used for the
 * data passed back and forth using TEE_IOC_CMD.
 */

/* Helpers to make the ioctl defines */
#define TEE_IOC_MAGIC	0xa4
#define TEE_IOC_BASE	0

/* Flags relating to shared memory */
#define TEE_IOCTL_SHM_MAPPED	0x1	/* memory mapped in normal world */
#define TEE_IOCTL_SHM_DMA_BUF	0x2	/* dma-buf handle on shared memory */

#define TEE_MAX_ARG_SIZE	1024

#define TEE_GEN_CAP_GP		(1 << 0)/* GlobalPlatform compliant TEE */
#define TEE_GEN_CAP_PRIVILEGED	(1 << 1)/* Privileged device (for supplicant) */

/*
 * TEE Implementation ID
  */
#define TEE_IMPL_ID_RSEE	1

#define TEE_RSEE_CAP_TZ	(1 << 0)

struct tee_ioctl_version_data {
 	__u32 impl_id;
	__u32 impl_caps;
	__u32 gen_caps;
// zhuhy, for version string
	__u8 tee_os_vstring[128];
	__u8 client_drv_vstring[128];
};

#define TEE_IOC_VERSION		_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 0, \
				  				     struct tee_ioctl_version_data)

struct tee_ioctl_shm_alloc_data {
 	__u64 size;
	__u32 flags;
	__s32 id;
 };

#define TEE_IOC_SHM_ALLOC	_IOWR(TEE_IOC_MAGIC, TEE_IOC_BASE + 1, \
				   				     struct tee_ioctl_shm_alloc_data)

struct tee_ioctl_shm_register_fd_data {
	__s64 fd;
	__u64 size;
	__u32 flags;
	__s32 id;
} __aligned(8);

#define TEE_IOC_SHM_REGISTER_FD	_IOWR(TEE_IOC_MAGIC, TEE_IOC_BASE + 8, \
				 				     struct tee_ioctl_shm_register_fd_data)

struct tee_ioctl_buf_data {
	__u64 buf_ptr;
	__u64 buf_len;
};

#define TEE_IOCTL_PARAM_ATTR_TYPE_NONE		0	/* parameter not used */

#define TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT	1
#define TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT	2
#define TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT	3	/* input and output */

#define TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT	5
#define TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT	6
#define TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT	7	/* input and output */

#define TEE_IOCTL_PARAM_ATTR_TYPE_MASK		0xff

/* Meta parameter carrying extra information about the message. */
#define TEE_IOCTL_PARAM_ATTR_META		0x100

/* Mask of all known attr bits */
#define TEE_IOCTL_PARAM_ATTR_MASK \
	(TEE_IOCTL_PARAM_ATTR_TYPE_MASK | TEE_IOCTL_PARAM_ATTR_META)

/*
 * Matches TEEC_LOGIN_* in GP TEE Client API
 * Are only defined for GP compliant TEEs
 */
#define TEE_IOCTL_LOGIN_PUBLIC			0
#define TEE_IOCTL_LOGIN_USER			1
#define TEE_IOCTL_LOGIN_GROUP			2
#define TEE_IOCTL_LOGIN_APPLICATION		4
#define TEE_IOCTL_LOGIN_USER_APPLICATION	5
#define TEE_IOCTL_LOGIN_GROUP_APPLICATION	6

struct tee_ioctl_param_memref {
	__u64 shm_offs;
	__u64 size;
	__s64 shm_id;
};

struct tee_ioctl_param_value {
	__u64 a;
	__u64 b;
	__u64 c;
};

struct tee_ioctl_param {
	__u64 attr;
	union {
		struct tee_ioctl_param_memref memref;
		struct tee_ioctl_param_value value;
	} u;
};

#define TEE_IOCTL_UUID_LEN		16

 struct tee_ioctl_open_session_arg {
	__u8 uuid[TEE_IOCTL_UUID_LEN];
	__u8 clnt_uuid[TEE_IOCTL_UUID_LEN];
	__u32 clnt_login;
	__u32 cancel_id;
	__u32 session;
	__u32 ret;
	__u32 ret_origin;
	__u32 num_params;
} __aligned(8);

#define TEE_IOC_OPEN_SESSION	_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 2, \
								     struct tee_ioctl_buf_data)

struct tee_ioctl_invoke_arg {
	__u32 func;
	__u32 session;
	__u32 cancel_id;
	__u32 ret;
	__u32 ret_origin;
	__u32 num_params;
} __aligned(8);

#define TEE_IOC_INVOKE		_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 3, \
			  				     struct tee_ioctl_buf_data)

struct tee_ioctl_cancel_arg {
	__u32 cancel_id;
	__u32 session;
};

#define TEE_IOC_CANCEL		_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 4, \
	  				     struct tee_ioctl_cancel_arg)

struct tee_ioctl_close_session_arg {
	__u32 session;
};

#define TEE_IOC_CLOSE_SESSION	_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 5, \
	 				     struct tee_ioctl_close_session_arg)

struct tee_iocl_supp_recv_arg {
	__u32 func;
	__u32 num_params;
} __aligned(8);

#define TEE_IOC_SUPPL_RECV	_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 6, \
			 				     struct tee_ioctl_buf_data)

struct tee_iocl_supp_send_arg {
	__u32 ret;
	__u32 num_params;
} __aligned(8);

#define TEE_IOC_SUPPL_SEND	_IOR(TEE_IOC_MAGIC, TEE_IOC_BASE + 7, \
		 				     struct tee_ioctl_buf_data)

#endif /*__TEE_H*/
