
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;
typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;
__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
__extension__ typedef long long int __intmax_t;
__extension__ typedef unsigned long long int __uintmax_t;
__extension__ typedef __uint64_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __uint64_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __int64_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __uint64_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;
__extension__ typedef int __daddr_t;
__extension__ typedef int __key_t;
__extension__ typedef int __clockid_t;
__extension__ typedef void * __timer_t;
__extension__ typedef long int __blksize_t;
__extension__ typedef long int __blkcnt_t;
__extension__ typedef __int64_t __blkcnt64_t;
__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __uint64_t __fsblkcnt64_t;
__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __uint64_t __fsfilcnt64_t;
__extension__ typedef int __fsword_t;
__extension__ typedef int __ssize_t;
__extension__ typedef long int __syscall_slong_t;
__extension__ typedef unsigned long int __syscall_ulong_t;
typedef __off64_t __loff_t;
typedef char *__caddr_t;
__extension__ typedef int __intptr_t;
__extension__ typedef unsigned int __socklen_t;
typedef int __sig_atomic_t;
__extension__ typedef __int64_t __time64_t;
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __loff_t loff_t;
typedef __ino_t ino_t;
typedef __dev_t dev_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __ssize_t ssize_t;
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;
typedef __key_t key_t;
typedef __clock_t clock_t;
typedef __clockid_t clockid_t;
typedef __time_t time_t;
typedef __timer_t timer_t;
typedef unsigned int size_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;
typedef int register_t __attribute__ ((__mode__ (__word__)));
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{
  return __builtin_bswap16 (__bsx);
}
static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{
  return __builtin_bswap32 (__bsx);
}
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{
  return __builtin_bswap64 (__bsx);
}
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}
static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}
static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
typedef __sigset_t sigset_t;
struct timeval
{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
struct timespec
{
  __time_t tv_sec;
  __syscall_slong_t tv_nsec;
};
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
typedef struct
  {
    __fd_mask __fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];
  } fd_set;
typedef __fd_mask fd_mask;

extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);

typedef __blksize_t blksize_t;
typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;
  unsigned char __flags;
  unsigned char __shared;
  unsigned char __pad1;
  unsigned char __pad2;
  int __cur_writer;
};
typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
struct __pthread_mutex_s
{
  int __lock ;
  unsigned int __count;
  int __owner;
  int __kind;
 
  unsigned int __nusers;
  __extension__ union
  {
    int __spins;
    __pthread_slist_t __list;
  };
 
};
struct __pthread_cond_s
{
  __extension__ union
  {
    __extension__ unsigned long long int __wseq;
    struct
    {
      unsigned int __low;
      unsigned int __high;
    } __wseq32;
  };
  __extension__ union
  {
    __extension__ unsigned long long int __g1_start;
    struct
    {
      unsigned int __low;
      unsigned int __high;
    } __g1_start32;
  };
  unsigned int __g_refs[2] ;
  unsigned int __g_size[2];
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
};
typedef unsigned long int pthread_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
union pthread_attr_t
{
  char __size[36];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef union
{
  struct __pthread_mutex_s __data;
  char __size[24];
  long int __align;
} pthread_mutex_t;
typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;
typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[32];
  long int __align;
} pthread_rwlock_t;
typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;
typedef volatile int pthread_spinlock_t;
typedef union
{
  char __size[20];
  long int __align;
} pthread_barrier_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;

typedef unsigned char bool;
typedef unsigned char uchar;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned int uintptr;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;
typedef float float32;
typedef double float64;
typedef float64 float_t;
typedef union dma64addr {
 struct {
  uint32 lo;
  uint32 hi;
 };
 struct {
  uint32 low;
  uint32 high;
 };
 struct {
  uint32 loaddr;
  uint32 hiaddr;
 };
 struct {
  uint32 low_addr;
  uint32 high_addr;
 };
} dma64addr_t;
typedef unsigned long dmaaddr_t;
typedef struct {
 dmaaddr_t addr;
 uint32 length;
} hnddma_seg_t;
typedef struct {
 void *oshdmah;
 uint origsize;
 uint nsegs;
 hnddma_seg_t segs[8];
} hnddma_seg_map_t;
extern uint32 gFWID;
typedef struct {
 uint16 M_SSID_ID;
 uint16 M_PRS_FRM_BYTESZ_ID;
 uint16 M_SSID_BYTESZ_ID;
 uint16 M_PMQADDINT_THSD_ID;
 uint16 M_RT_BBRSMAP_A_ID;
 uint16 M_RT_BBRSMAP_B_ID;
 uint16 M_FIFOSIZE0_ID;
 uint16 M_FIFOSIZE1_ID;
 uint16 M_FIFOSIZE2_ID;
 uint16 M_FIFOSIZE3_ID;
 uint16 M_MBURST_TXOP_ID;
 uint16 M_MBURST_SIZE_ID;
 uint16 M_MODE_CORE_ID;
 uint16 M_WLCX_CONFIG_ID;
 uint16 M_WLCX_GPIO_CONFIG_ID;
 uint16 M_DOT11_DTIMPERIOD_ID;
 uint16 M_TIMBC_OFFSET_ID;
 uint16 M_CCKBOOST_ADJ_ID;
 uint16 M_EDCF_BLKS_ID;
 uint16 M_EDCF_QINFO1_ID;
 uint16 M_EDCF_QINFO2_ID;
 uint16 M_EDCF_QINFO3_ID;
 uint16 M_EDCF_BOFF_CTR_ID;
 uint16 M_LNA_ROUT_ID;
 uint16 M_RXGAIN_HI_ID;
 uint16 M_RXGAIN_LO_ID;
 uint16 M_RXGAINOVR2_VAL_ID;
 uint16 M_LPFGAIN_HI_ID;
 uint16 M_LPFGAIN_LO_ID;
 uint16 M_CUR_TXF_INDEX_ID;
 uint16 M_COREMASK_BLK_ID;
 uint16 M_COREMASK_BPHY_ID;
 uint16 M_COREMASK_BFM_ID;
 uint16 M_COREMASK_BFM1_ID;
 uint16 M_COREMASK_BTRESP_ID;
 uint16 M_TXPWR_BLK_ID;
 uint16 M_PS_MORE_DTIM_TBTT_ID;
 uint16 M_POSTDTIM0_NOSLPTIME_ID;
 uint16 M_BCMC_FID_ID;
 uint16 M_AMT_INFO_PTR_ID;
 uint16 M_PRS_MAXTIME_ID;
 uint16 M_PRETBTT_ID;
 uint16 M_BCN_TXTSF_OFFSET_ID;
 uint16 M_TIMBPOS_INBEACON_ID;
 uint16 M_AGING_THRSH_ID;
 uint16 M_SYNTHPU_DELAY_ID;
 uint16 M_PHYVER_ID;
 uint16 M_PHYTYPE_ID;
 uint16 M_MAX_ANTCNT_ID;
 uint16 M_MACHW_VER_ID;
 uint16 M_MACHW_CAP_L_ID;
 uint16 M_MACHW_CAP_H_ID;
 uint16 M_SFRMTXCNTFBRTHSD_ID;
 uint16 M_LFRMTXCNTFBRTHSD_ID;
 uint16 M_TXDC_BYTESZ_ID;
 uint16 M_TXFL_BMAP_ID;
 uint16 M_JSSI_AUX_ID;
 uint16 M_DOT11_SLOT_ID;
 uint16 M_DUTY_STRTRATE_ID;
 uint16 M_SECRSSI0_MIN_ID;
 uint16 M_SECRSSI1_MIN_ID;
 uint16 M_MIMO_ANTSEL_RXDFLT_ID;
 uint16 M_MIMO_ANTSEL_TXDFLT_ID;
 uint16 M_ANTSEL_CLKDIV_ID;
 uint16 M_BOM_REV_MAJOR_ID;
 uint16 M_BOM_REV_MINOR_ID;
 uint16 M_HOST_FLAGS_ID;
 uint16 M_HOST_FLAGS2_ID;
 uint16 M_HOST_FLAGS3_ID;
 uint16 M_HOST_FLAGS4_ID;
 uint16 M_HOST_FLAGS5_ID;
 uint16 M_HOST_FLAGS6_ID;
 uint16 M_REV_L_ID;
 uint16 M_REV_H_ID;
 uint16 M_UCODE_FEATURES_ID;
 uint16 M_ULP_STATUS_ID;
 uint16 M_EDCF_QINFO1_OFFSET_ID;
 uint16 M_MYMAC_ADDR_ID;
 uint16 M_UTRACE_SPTR_ID;
 uint16 M_UTRACE_EPTR_ID;
 uint16 M_UTRACE_STS_ID;
 uint16 M_TXFRAME_CNT_ID;
 uint16 M_TXAMPDU_CNT_ID;
 uint16 M_RXSTRT_CNT_ID;
 uint16 M_RXCRSGLITCH_CNT_ID;
 uint16 M_BPHYGLITCH_CNT_ID;
 uint16 M_RXBADFCS_CNT_ID;
 uint16 M_RXBADPLCP_CNT_ID;
 uint16 M_RXBPHY_BADPLCP_CNT_ID;
 uint16 M_AC_TXLMT_BLK_ID;
 uint16 M_MAXRXFRM_LEN_ID;
 uint16 M_MAXRXFRM_LEN2_ID;
 uint16 M_MAXRXMPDU_LEN_ID;
 uint16 M_RXCORE_STATE_ID;
 uint16 M_ASSERT_REASON_ID;
 uint16 M_WRDCNT_RXF1OVFL_THRSH_ID;
 uint16 M_WRDCNT_RXF0OVFL_THRSH_ID;
 uint16 M_NCAL_ACTIVE_CORES_ID;
 uint16 M_NCAL_CFG_BMP_ID;
 uint16 M_NCAL_RFCTRLOVR_0_ID;
 uint16 M_NCAL_RXGAINOVR_0_ID;
 uint16 M_NCAL_RXLPFOVR_0_ID;
 uint16 M_NCAL_RXGAIN_LO_ID;
 uint16 M_NCAL_LPFGAIN_LO_ID;
 uint16 M_NCAL_RXGAIN_HI_ID;
 uint16 M_NCAL_LPFGAIN_HI_ID;
 uint16 M_NCAL_RFCTRLOVR_VAL_ID;
 uint16 M_BSR_BLK_ID;
 uint16 M_SEMAPHORE_BSR_ID;
 uint16 M_BSR_ALLTID_SET0_ID;
 uint16 M_BSR_ALLTID_SET1_ID;
 uint16 M_ACKPWRTBL_R0_1_ID;
 uint16 M_ACKPWRTBL_R0_2_ID;
 uint16 M_ACKPWRTBL_R1_0_ID;
 uint16 M_ACKPWRTBL_R1_1_ID;
 uint16 M_ACKPWRTBL_R2_0_ID;
 uint16 M_ACKPWRTBL_R2_1_ID;
 uint16 M_ACKPWRTBL_R3_0_ID;
 uint16 M_ACKPWRTBL_R3_1_ID;
 uint16 M_ACKPWRTBL_R4_0_ID;
 uint16 M_ACKPWRTBL_R4_1_ID;
 uint16 M_ACKPWRTBL_R5_0_ID;
 uint16 M_ACKPWRTBL_R5_1_ID;
 uint16 M_ACKPWRTBL_R6_0_ID;
 uint16 M_ACKPWRTBL_R6_1_ID;
 uint16 M_ACKPWRTBL_R7_0_ID;
 uint16 M_ACKPWRTBL_R7_1_ID;
 uint16 M_ACKPWRTBL_R8_0_ID;
 uint16 M_ACKPWRTBL_R8_1_ID;
 uint16 M_ACK_2GCM_NUMB_ID;
 uint16 M_BTACK_2GCM_NUMB_ID;
 uint16 M_CUR_ACK_OFST_ID;
 uint16 M_TXPWRCAP_C0_ID;
 uint16 M_TXPWRCAP_C1_ID;
 uint16 M_TXPWRCAP_C2_ID;
 uint16 M_TXPWRCAP_C0_FS_ID;
 uint16 M_TXPWRCAP_C1_FS_ID;
 uint16 M_TXPWRCAP_C2_FS_ID;
 uint16 M_TXPWRCAP_C0_FSANT_ID;
 uint16 M_TXPWRCAP_BT_C0_ID;
 uint16 M_TXPWRCAP_BT_C1_ID;
 uint16 M_TXPWRCAP_BT_C2_ID;
 uint16 M_TDMTX_RSSI_THR_ID;
 uint16 M_TDMTX_TXPWRBOFF_ID;
 uint16 M_TDMTX_TXPWRBOFF_DT_ID;
 uint16 M_CCA_MEASINTV_L_ID;
 uint16 M_CCA_MEASINTV_H_ID;
 uint16 M_CCA_MEASBUSY_L_ID;
 uint16 M_CCA_MEASBUSY_H_ID;
} d11shm_common_base_t;
typedef struct {
 uint16 M_TOF_BLK_PTR_ID;
 uint16 M_TOF_CMD_OFFSET_ID;
 uint16 M_TOF_RSP_OFFSET_ID;
 uint16 M_TOF_CHNSM_0_OFFSET_ID;
 uint16 M_TOF_DOT11DUR_OFFSET_ID;
 uint16 M_TOF_PHYCTL0_OFFSET_ID;
 uint16 M_TOF_PHYCTL1_OFFSET_ID;
 uint16 M_TOF_PHYCTL2_OFFSET_ID;
 uint16 M_TOF_PHYCTL0_ID;
 uint16 M_TOF_PHYCTL1_ID;
 uint16 M_TOF_PHYCTL2_ID;
 uint16 M_TOF_HT_PHYCTL0_ID;
 uint16 M_TOF_HT_PHYCTL1_ID;
 uint16 M_TOF_HT_PHYCTL2_ID;
 uint16 M_TOF_HE_PHYCTL0_ID;
 uint16 M_TOF_HE_PHYCTL1_ID;
 uint16 M_TOF_HE_PHYCTL2_ID;
 uint16 M_TOF_LSIG_OFFSET_ID;
 uint16 M_TOF_VHTA0_OFFSET_ID;
 uint16 M_TOF_VHTA1_OFFSET_ID;
 uint16 M_TOF_VHTA2_OFFSET_ID;
 uint16 M_TOF_VHTA0_ID;
 uint16 M_TOF_VHTA1_ID;
 uint16 M_TOF_VHTA2_ID;
 uint16 M_TOF_VHTB0_OFFSET_ID;
 uint16 M_TOF_VHTB1_OFFSET_ID;
 uint16 M_TOF_HTSIG0_ID;
 uint16 M_TOF_HTSIG1_ID;
 uint16 M_TOF_HTSIG2_ID;
 uint16 M_TOF_HESIG0_ID;
 uint16 M_TOF_HESIG1_ID;
 uint16 M_TOF_HESIG2_ID;
 uint16 M_TOF_FLAGS_OFFSET_ID;
 uint16 M_TOF_FLAGS_ID;
 uint16 M_TOF_AVB_BLK_ID;
 uint16 M_TOF_AVB_BLK_IDX_ID;
 uint16 M_TOF_DBG_BLK_OFFSET_ID;
 uint16 M_TOF_DBG1_OFFSET_ID;
 uint16 M_TOF_DBG2_OFFSET_ID;
 uint16 M_TOF_DBG3_OFFSET_ID;
 uint16 M_TOF_DBG4_OFFSET_ID;
 uint16 M_TOF_DBG5_OFFSET_ID;
 uint16 M_TOF_DBG6_OFFSET_ID;
 uint16 M_TOF_DBG7_OFFSET_ID;
 uint16 M_TOF_DBG8_OFFSET_ID;
 uint16 M_TOF_DBG9_OFFSET_ID;
 uint16 M_TOF_DBG10_OFFSET_ID;
 uint16 M_TOF_DBG11_OFFSET_ID;
 uint16 M_TOF_DBG12_OFFSET_ID;
 uint16 M_TOF_DBG13_OFFSET_ID;
 uint16 M_TOF_DBG14_OFFSET_ID;
 uint16 M_TOF_DBG15_OFFSET_ID;
 uint16 M_TOF_DBG16_OFFSET_ID;
 uint16 M_TOF_DBG17_OFFSET_ID;
 uint16 M_TOF_DBG18_OFFSET_ID;
 uint16 M_TOF_DBG19_OFFSET_ID;
 uint16 M_TOF_DBG20_OFFSET_ID;
 uint16 M_FTM_SC_CURPTR_ID;
 uint16 M_FTM_NYSM0_ID;
} d11shm_proxd_t;
typedef struct {
 uint16 M_P2P_BLK_PTR_ID;
 uint16 M_P2P_PERBSS_BLK_ID;
 uint16 M_P2P_INTF_BLK_ID;
 uint16 M_ADDR_BMP_BLK_OFFSET_ID;
 uint16 M_P2P_INTR_BLK_ID;
 uint16 M_P2P_HPS_ID;
 uint16 M_P2P_HPS_OFFSET_ID;
 uint16 M_P2P_TSF_OFFSET_BLK_ID;
 uint16 M_P2P_GO_CHANNEL_ID;
 uint16 M_P2P_GO_IND_BMP_ID;
 uint16 M_P2P_PRETBTT_BLK_ID;
 uint16 M_P2P_TSF_DRIFT_WD0_ID;
 uint16 M_P2P_TXSTOP_T_BLK_ID;
} d11shm_mcnx_t;
typedef struct {
 uint16 M_SHM_BYT_CNT_ID;
} d11shm_p2p_t;
typedef struct {
 uint16 M_MFGTEST_FRMCNT_LO_ID;
 uint16 M_MFGTEST_FRMCNT_HI_ID;
 uint16 M_MFGTEST_NUM_ID;
 uint16 M_MFGTEST_IO1_ID;
 uint16 M_RXSTATS_BLK_PTR_ID;
 uint16 M_MFGTEST_UOTARXTEST_ID;
 uint16 M_MFGTEST_RXAVGPWR_ANT0_ID;
 uint16 M_MFGTEST_RXAVGPWR_ANT1_ID;
 uint16 M_MFGTEST_RXAVGPWR_ANT2_ID;
} d11shm_mfgtest_t;
typedef struct {
 uint16 M_TKMICKEYS_PTR_ID;
 uint16 M_WAPIMICKEYS_PTR_ID;
 uint16 M_SECKEYS_PTR_ID;
 uint16 M_SECKINDX_PTR_ID;
 uint16 M_TKIP_TTAK_PTR_ID;
 uint16 M_SECKINDXALGO_BLK_ID;
 uint16 M_TKIP_TSC_TTAK_ID;
} d11shm_km_t;
typedef struct {
 uint16 M_BFI_BLK_ID;
 uint16 M_BFI_REFRESH_THR_ID;
 uint16 M_BFI_NRXC_ID;
} d11shm_txbf_t;
typedef struct {
 uint16 M_PSM2HOST_STATS_ID;
 uint16 M_PSM2HOST_EXT_PTR_ID;
 uint16 M_UCODE_DBGST_ID;
 uint16 M_BCN0_FRM_BYTESZ_ID;
 uint16 M_BCN1_FRM_BYTESZ_ID;
 uint16 M_DEBUGBLK_PTR_ID;
 uint16 M_TXERR_NOWRITE_ID;
 uint16 M_TXERR_REASON_ID;
 uint16 M_TXERR_REASON2_ID;
 uint16 M_TXERR_CTXTST_ID;
 uint16 M_TXERR_PCTL0_ID;
 uint16 M_TXERR_PCTL1_ID;
 uint16 M_TXERR_PCTL2_ID;
 uint16 M_TXERR_LSIG0_ID;
 uint16 M_TXERR_LSIG1_ID;
 uint16 M_TXERR_PLCP0_ID;
 uint16 M_TXERR_PLCP1_ID;
 uint16 M_TXERR_PLCP2_ID;
 uint16 M_TXERR_SIGB0_ID;
 uint16 M_TXERR_SIGB1_ID;
 uint16 M_TXERR_RXESTATS2_ID;
 uint16 M_TXNDPA_CNT_ID;
 uint16 M_TXNDP_CNT_ID;
 uint16 M_TXBFPOLL_CNT_ID;
 uint16 M_TXSF_CNT_ID;
 uint16 M_VHTSU_TXNDPA_CNT_ID;
 uint16 M_VHTMU_TXNDPA_CNT_ID;
 uint16 M_HESU_TXNDPA_CNT_ID;
 uint16 M_HEMU_TXNDPA_CNT_ID;
 uint16 M_VHTSU_RXSF_CNT_ID;
 uint16 M_VHTMU_RXSF_CNT_ID;
 uint16 M_HESU_RXSF_CNT_ID;
 uint16 M_HEMU_RXSF_CNT_ID;
 uint16 M_VHTSU_EXPRXSF_CNT_ID;
 uint16 M_VHTMU_EXPRXSF_CNT_ID;
 uint16 M_HESU_EXPRXSF_CNT_ID;
 uint16 M_HEMU_EXPRXSF_CNT_ID;
 uint16 M_TXCWRTS_CNT_ID;
 uint16 M_TXCWCTS_CNT_ID;
 uint16 M_TXBFM_CNT_ID;
 uint16 M_RXNDPAUCAST_CNT_ID;
 uint16 M_RXNDPAMCAST_CNT_ID;
 uint16 M_RXBFPOLLUCAST_CNT_ID;
 uint16 M_BFERPTRDY_CNT_ID;
 uint16 M_RXSFUCAST_CNT_ID;
 uint16 M_RXCWRTSUCAST_CNT_ID;
 uint16 M_RXCWCTSUCAST_CNT_ID;
 uint16 M_RXDROP20S_CNT_ID;
 uint16 M_RX20S_CNT_ID;
 uint16 M_BTCX_RFACT_CTR_L_ID;
 uint16 M_MACSUSP_CNT_ID;
} d11shm_macstat_t;
typedef struct {
 uint16 M_MBSSID_BLK_ID;
 uint16 M_MBS_BSSID0_ID;
 uint16 M_MBS_BSSID1_ID;
 uint16 M_MBS_BSSID2_ID;
 uint16 M_MBS_NBCN_ID;
 uint16 M_MBS_PRQBASE_ID;
 uint16 M_MBS_PRETBTT_ID;
 uint16 M_MBS_BSSIDNUM_ID;
 uint16 M_MBS_PIO_BCBMP_ID;
 uint16 M_MBS_PRS_TPLPTR_ID;
 uint16 M_MBS_PRSLEN_BLK_ID;
 uint16 M_MBS_BCFID_BLK_ID;
 uint16 M_MBS_SSIDLEN_BLK_ID;
 uint16 M_MBS_SSID_1_ID;
 uint16 M_PRQFIFO_RPTR_ID;
 uint16 M_PRQFIFO_WPTR_ID;
 uint16 M_BCN_TPLBLK_BSZ_ID;
} d11shm_mbssid_t;
typedef struct {
 uint16 M_RFLDO_ON_L_ID;
 uint16 M_RFLDO_ON_H_ID;
 uint16 M_PAPDOFF_MCS_ID;
 uint16 M_LPF_PASSIVE_RC_OFDM_ID;
 uint16 M_LPF_PASSIVE_RC_CCK_ID;
 uint16 M_SMPL_COL_BMP_ID;
 uint16 M_SMPL_COL_CTL_ID;
 uint16 M_RT_DIRMAP_A_ID;
 uint16 M_RT_DIRMAP_B_ID;
 uint16 M_CTS_DURATION_ID;
 uint16 M_TSSI_0_ID;
 uint16 M_TSSI_1_ID;
 uint16 M_IFS_PRICRS_ID;
 uint16 M_TSSI_OFDM_0_ID;
 uint16 M_TSSI_OFDM_1_ID;
 uint16 M_CURCHANNEL_ID;
 uint16 M_PWRIND_BLKS_ID;
 uint16 M_PWRIND_MAP0_ID;
 uint16 M_PWRIND_MAP1_ID;
 uint16 M_PWRIND_MAP2_ID;
 uint16 M_PWRIND_MAP3_ID;
 uint16 M_PWRIND_MAP4_ID;
 uint16 M_PWRIND_MAP5_ID;
 uint16 M_HWACI_STATUS_ID;
 uint16 M_HWACI_EN_IND_ID;
 uint16 M_FCBS_BLK_ID;
 uint16 M_FCBS_TEMPLATE_LENS_ID;
 uint16 M_FCBS_BPHY_CTRL_ID;
 uint16 M_FCBS_TEMPLATE_PTR_ID;
 uint16 M_AFEOVR_PTR_ID;
 uint16 M_RXGAININFO_ANT0_OFFSET_ID;
 uint16 M_RXAUXGAININFO_ANT0_OFFSET_ID;
 uint16 M_RADAR_REG_ID;
} d11shm_physim_t;
typedef struct {
 uint16 M_RADIOPWRUP_PTR_ID;
 uint16 M_RADIOPWRDWN_PTR_ID;
 uint16 M_EXT_AVAIL3_ID;
 uint16 M_TXPWR_MAX_ID;
 uint16 M_TXPWR_CUR_ID;
 uint16 M_BCMC_TIMEOUT_ID;
 uint16 M_BCMCROLL_TMOUT_ID;
 uint16 M_TXPWR_M_ID;
 uint16 M_TXPWR_TARGET_ID;
 uint16 M_USEQ_PWRUP_PTR_ID;
 uint16 M_USEQ_PWRDN_PTR_ID;
 uint16 M_SLP_RDY_INT_ID;
 uint16 M_TPCNNUM_INTG_LOG2_ID;
 uint16 M_TSSI_SENS_LMT1_ID;
 uint16 M_TSSI_SENS_LMT2_ID;
} d11shm_ulpphysim_t;
typedef struct {
 uint16 M_BTCX_BLK_PTR_ID;
 uint16 M_BTCX_PRED_PER_ID;
 uint16 M_BTCX_LAST_SCO_ID;
 uint16 M_BTCX_LAST_SCO_H_ID;
 uint16 M_BTCX_BLE_SCAN_GRANT_THRESH_ID;
 uint16 M_BTCX_NEXT_SCO_ID;
 uint16 M_BTCX_REQ_START_ID;
 uint16 M_BTCX_REQ_START_H_ID;
 uint16 M_BTCX_LAST_DATA_ID;
 uint16 M_BTCX_BT_TYPE_ID;
 uint16 M_BTCX_ECI0_ID;
 uint16 M_BTCX_ECI1_ID;
 uint16 M_BTCX_ECI2_ID;
 uint16 M_BTCX_ECI3_ID;
 uint16 M_BTCX_LAST_A2DP_ID;
 uint16 M_BTCX_PRI_MAP_LO_ID;
 uint16 M_BTCX_HOLDSCO_LIMIT_OFFSET_ID;
 uint16 M_BTCX_SCO_GRANT_HOLD_RATIO_OFFSET_ID;
 uint16 M_BTCX_PRED_COUNT_ID;
 uint16 M_BTCX_PROT_RSSI_THRESH_OFFSET_ID;
 uint16 M_BTCX_PROT_RSSI_THRESH_ID;
 uint16 M_BTCX_HOST_FLAGS_OFFSET_ID;
 uint16 M_BTCX_HOST_FLAGS_ID;
 uint16 M_BTCX_RSSI_ID;
 uint16 M_BTCX_BT_TXPWR_ID;
 uint16 M_BTCX_HIGH_THRESH_ID;
 uint16 M_BTCX_LOW_THRESH_ID;
 uint16 M_BTCX_CONFIG_OFFSET_ID;
 uint16 M_BTCX_CONFIG_ID;
 uint16 M_BTCX_NUM_TASKS_OFFSET_ID;
 uint16 M_BTCX_NUM_TASKS_ID;
 uint16 M_BTCXDBG_BLK_ID;
 uint16 M_BTCX_RFSWMSK_BT_OFFSET_ID;
 uint16 M_BTCX_RFSWMSK_BT_ID;
 uint16 M_BTCX_RFSWMSK_WL_OFFSET_ID;
 uint16 M_BTCX_RFSWMSK_WL_ID;
 uint16 M_BTCX_AGG_OFF_BM_ID;
 uint16 M_BTCX_PKTABORTCTL_VAL_ID;
 uint16 M_BTCX_BT_TASKS_BM_LOW_ID;
 uint16 M_BTCX_BT_TASKS_BM_HI_ID;
 uint16 M_BTCX_ABORT_CNT_ID;
 uint16 M_BTCX_LATENCY_CNT_ID;
 uint16 M_BTCX_HOST_FLAGS2_OFFSET_ID;
 uint16 M_BTCX_HOST_FLAGS2_ID;
 uint16 M_BTCX_IBSS_TSF_L_ID;
 uint16 M_BTCX_IBSS_TSF_ML_ID;
 uint16 M_BTCX_IBSS_TSF_SCO_L_ID;
 uint16 M_BTCX_SUCC_PM_PROTECT_CNT_ID;
 uint16 M_BTCX_SUCC_CTS2A_CNT_ID;
 uint16 M_BTCX_WLAN_TX_PREEMPT_CNT_ID;
 uint16 M_BTCX_WLAN_RX_PREEMPT_CNT_ID;
 uint16 M_BTCX_APTX_AFTER_PM_CNT_ID;
 uint16 M_BTCX_PERAUD_CUMU_GRANT_CNT_ID;
 uint16 M_BTCX_PERAUD_CUMU_DENY_CNT_ID;
 uint16 M_BTCX_A2DP_CUMU_GRANT_CNT_ID;
 uint16 M_BTCX_A2DP_CUMU_DENY_CNT_ID;
 uint16 M_BTCX_SNIFF_CUMU_GRANT_CNT_ID;
 uint16 M_BTCX_SNIFF_CUMU_DENY_CNT_ID;
} d11shm_btcx_t;
typedef struct {
 uint16 M_LTECX_FLAGS_ID;
 uint16 M_LTECX_STATE_ID;
 uint16 M_LTECX_HOST_FLAGS_ID;
 uint16 M_LTECX_TX_LOOKAHEAD_DUR_ID;
 uint16 M_LTECX_PROT_ADV_TIME_ID;
 uint16 M_LTECX_WCI2_TST_LPBK_NBYTES_TX_ID;
 uint16 M_LTECX_WCI2_TST_LPBK_NBYTES_ERR_ID;
 uint16 M_LTECX_WCI2_TST_LPBK_NBYTES_RX_ID;
 uint16 M_LTECX_RX_REAGGR_ID;
 uint16 M_LTECX_ACTUALTX_DURATION_ID;
 uint16 M_LTECX_CRTI_MSG_ID;
 uint16 M_LTECX_CRTI_INTERVAL_ID;
 uint16 M_LTECX_CRTI_REPEATS_ID;
 uint16 M_LTECX_WCI2_TST_MSG_ID;
 uint16 M_LTECX_RXPRI_THRESH_ID;
 uint16 M_LTECX_MWSSCAN_BM_LO_ID;
 uint16 M_LTECX_MWSSCAN_BM_HI_ID;
 uint16 M_LTECX_PWRCP_C0_ID;
 uint16 M_LTECX_PWRCP_C0_FS_ID;
 uint16 M_LTECX_PWRCP_C0_FSANT_ID;
 uint16 M_LTECX_PWRCP_C1_ID;
 uint16 M_LTECX_PWRCP_C1_FS_ID;
 uint16 M_LTECX_FS_OFFSET_ID;
 uint16 M_LTECX_TXNOISE_CNT_ID;
 uint16 M_LTECX_NOISE_DELTA_ID;
 uint16 M_LTECX_TYPE4_TXINHIBIT_DURATION_ID;
 uint16 M_LTECX_TYPE4_NONE_ZERO_CNT_ID;
 uint16 M_LTECX_TYPE4_TIMEOUT_CNT_ID;
 uint16 M_LTECX_RXPRI_DURATION_ID;
 uint16 M_LTECX_RXPRI_CNT_ID;
 uint16 M_LTECX_TYP6_DURATION_ID;
 uint16 M_LTECX_TYP6_CNT_ID;
 uint16 M_LTECX_TS_PROT_FRAME_CNT_ID;
 uint16 M_LTECX_TS_GRANT_CNT_ID;
 uint16 M_LTECX_TS_GRANT_TIME_DUR_ID;
} d11shm_ltecx_t;
typedef struct {
 uint16 M_CCASTATS_PTR_ID;
 uint16 M_CCA_STATS_BLK_ID;
 uint16 M_CCA_TXDUR_L_OFFSET_ID;
 uint16 M_CCA_TXDUR_H_OFFSET_ID;
 uint16 M_CCA_INBSS_L_OFFSET_ID;
 uint16 M_CCA_INBSS_H_OFFSET_ID;
 uint16 M_CCA_OBSS_L_OFFSET_ID;
 uint16 M_CCA_OBSS_H_OFFSET_ID;
 uint16 M_CCA_NOCTG_L_OFFSET_ID;
 uint16 M_CCA_NOCTG_H_OFFSET_ID;
 uint16 M_CCA_NOPKT_L_OFFSET_ID;
 uint16 M_CCA_NOPKT_H_OFFSET_ID;
 uint16 M_MAC_SLPDUR_L_OFFSET_ID;
 uint16 M_MAC_SLPDUR_H_OFFSET_ID;
 uint16 M_CCA_TXOP_L_OFFSET_ID;
 uint16 M_CCA_TXOP_H_OFFSET_ID;
 uint16 M_CCA_GDTXDUR_L_OFFSET_ID;
 uint16 M_CCA_GDTXDUR_H_OFFSET_ID;
 uint16 M_CCA_BDTXDUR_L_OFFSET_ID;
 uint16 M_CCA_BDTXDUR_H_OFFSET_ID;
 uint16 M_CCA_WIFI_L_OFFSET_ID;
 uint16 M_CCA_WIFI_H_OFFSET_ID;
 uint16 M_CCA_EDCRSDUR_L_OFFSET_ID;
 uint16 M_CCA_EDCRSDUR_H_OFFSET_ID;
 uint16 M_CCA_TXDUR_L_ID;
 uint16 M_CCA_TXDUR_H_ID;
 uint16 M_CCA_INBSS_L_ID;
 uint16 M_CCA_INBSS_H_ID;
 uint16 M_CCA_OBSS_L_ID;
 uint16 M_CCA_OBSS_H_ID;
 uint16 M_CCA_NOCTG_L_ID;
 uint16 M_CCA_NOCTG_H_ID;
 uint16 M_CCA_NOPKT_L_ID;
 uint16 M_CCA_NOPKT_H_ID;
 uint16 M_MAC_SLPDUR_L_ID;
 uint16 M_MAC_SLPDUR_H_ID;
 uint16 M_CCA_TXOP_L_ID;
 uint16 M_CCA_TXOP_H_ID;
 uint16 M_CCA_GDTXDUR_L_ID;
 uint16 M_CCA_GDTXDUR_H_ID;
 uint16 M_CCA_BDTXDUR_L_ID;
 uint16 M_CCA_BDTXDUR_H_ID;
 uint16 M_CCA_RXPRI_LO_ID;
 uint16 M_CCA_RXPRI_HI_ID;
 uint16 M_CCA_RXSEC20_LO_ID;
 uint16 M_CCA_RXSEC20_HI_ID;
 uint16 M_CCA_RXSEC40_LO_ID;
 uint16 M_CCA_RXSEC40_HI_ID;
 uint16 M_CCA_RXSEC80_LO_ID;
 uint16 M_CCA_RXSEC80_HI_ID;
 uint16 M_CCA_SUSP_L_ID;
 uint16 M_CCA_SUSP_H_ID;
 uint16 M_CCA_WIFI_L_ID;
 uint16 M_CCA_WIFI_H_ID;
 uint16 M_CCA_EDCRSDUR_L_ID;
 uint16 M_CCA_EDCRSDUR_H_ID;
 uint16 M_SECRSSI0_ID;
 uint16 M_SECRSSI1_ID;
 uint16 M_SECRSSI2_ID;
 uint16 M_SISO_RXDUR_L_ID;
 uint16 M_SISO_RXDUR_H_ID;
 uint16 M_SISO_TXOP_L_ID;
 uint16 M_SISO_TXOP_H_ID;
 uint16 M_MIMO_RXDUR_L_ID;
 uint16 M_MIMO_RXDUR_H_ID;
 uint16 M_MIMO_TXOP_L_ID;
 uint16 M_MIMO_TXOP_H_ID;
 uint16 M_MIMO_TXDUR_1X_L_ID;
 uint16 M_MIMO_TXDUR_1X_H_ID;
 uint16 M_MIMO_TXDUR_2X_L_ID;
 uint16 M_MIMO_TXDUR_2X_H_ID;
 uint16 M_MIMO_TXDUR_3X_L_ID;
 uint16 M_MIMO_TXDUR_3X_H_ID;
 uint16 M_SISO_SIFS_L_ID;
 uint16 M_SISO_SIFS_H_ID;
 uint16 M_MIMO_SIFS_L_ID;
 uint16 M_MIMO_SIFS_H_ID;
 uint16 M_CCA_TXNODE0_L_OFFSET_ID;
 uint16 M_CCA_TXNODE0_H_OFFSET_ID;
 uint16 M_CCA_RXNODE0_L_OFFSET_ID;
 uint16 M_CCA_RXNODE0_H_OFFSET_ID;
 uint16 M_CCA_XXOBSS_L_OFFSET_ID;
 uint16 M_CCA_XXOBSS_H_OFFSET_ID;
 uint16 M_MESHCCA_STATS_BLK_ID;
 uint16 M_MESHCCA_TXNODE0_OFFSET_ID;
 uint16 M_MESHCCA_RXNODE0_OFFSET_ID;
 uint16 M_MESHCCA_OBSSNODE_OFFSET_ID;
 uint16 M_MESHCCA_TXNODE0_ID;
 uint16 M_MESHCCA_RXNODE0_ID;
 uint16 M_MESHCCA_OBSSNODE_ID;
 uint16 M_HUGENAV_RTS_CNT_ID;
 uint16 M_HUGENAV_CTS_CNT_ID;
} d11shm_ccastat_t;
typedef struct {
 uint16 M_TXFS_PTR_ID;
 uint16 M_AMP_STATS_PTR_ID;
 uint16 M_MIMO_MAXSYM_ID;
 uint16 M_WATCHDOG_8TU_ID;
} d11shm_ampdu_t;
typedef struct {
 uint16 M_BTAMP_GAIN_DELTA_ID;
} d11shm_btamp_t;
typedef struct {
 uint16 M_BCN_TXPCTL0_ID;
 uint16 M_BCN_TXPCTL1_ID;
 uint16 M_BCN_TXPCTL2_ID;
 uint16 M_RSP_TXPCTL0_ID;
 uint16 M_RSP_TXPCTL1_ID;
 uint16 M_UPRS_INTVL_L_ID;
 uint16 M_UPRS_INTVL_H_ID;
 uint16 M_UPRS_FD_TSF_LOC_ID;
 uint16 M_UPRS_FD_TXTSF_OFFSET_ID;
 uint16 M_TXDUTY_RATIOX16_CCK_ID;
 uint16 M_TXDUTY_RATIOX16_OFDM_ID;
} d11shm_rev_ge_40_t;
typedef struct {
 uint16 M_BCN_PCTLWD_ID;
 uint16 M_BCN_PCTL1WD_ID;
 uint16 M_SWDIV_SWCTRL_REG_ID;
 uint16 M_SWDIV_PREF_ANT_ID;
 uint16 M_SWDIV_TX_PREF_ANT_ID;
 uint16 M_LCNXN_SWCTRL_MASK_ID;
 uint16 M_4324_RXTX_WAR_PTR_ID;
 uint16 M_TX_MODE_0xb0_OFFSET_ID;
 uint16 M_TX_MODE_0x14d_OFFSET_ID;
 uint16 M_TX_MODE_0xb1_OFFSET_ID;
 uint16 M_TX_MODE_0x14e_OFFSET_ID;
 uint16 M_TX_MODE_0xb4_OFFSET_ID;
 uint16 M_TX_MODE_0x151_OFFSET_ID;
 uint16 M_RX_MODE_0xb0_OFFSET_ID;
 uint16 M_RX_MODE_0x14d_OFFSET_ID;
 uint16 M_RX_MODE_0xb1_OFFSET_ID;
 uint16 M_RX_MODE_0x14e_OFFSET_ID;
 uint16 M_RX_MODE_0xb4_OFFSET_ID;
 uint16 M_RX_MODE_0x151_OFFSET_ID;
 uint16 M_CHIP_CHECK_OFFSET_ID;
 uint16 M_CTXPRS_BLK_ID;
 uint16 M_RADIO_PWR_ID;
 uint16 M_IFSCTL1_ID;
 uint16 M_TX_IDLE_BUSY_RATIO_X_16_CCK_ID;
 uint16 M_TX_IDLE_BUSY_RATIO_X_16_OFDM_ID;
 uint16 M_RSP_PCTLWD_ID;
 uint16 M_BCN_POWER_ADJUST_ID;
 uint16 M_PRS_POWER_ADJUST_ID;
} d11shm_rev_lt_40_t;
typedef struct {
 uint16 M_BCNTRIM_BLK_ID;
 uint16 M_BCNTRIM_PER_OFFSET_ID;
 uint16 M_BCNTRIM_TIMEND_OFFSET_ID;
 uint16 M_BCNTRIM_TSFLMT_OFFSET_ID;
 uint16 M_BCNTRIM_CNT_OFFSET_ID;
 uint16 M_BCNTRIM_RSSI_OFFSET_ID;
 uint16 M_BCNTRIM_CHAN_OFFSET_ID;
 uint16 M_BCNTRIM_SNR_OFFSET_ID;
 uint16 M_BCNTRIM_CUR_OFFSET_ID;
 uint16 M_BCNTRIM_PREVLEN_OFFSET_ID;
 uint16 M_BCNTRIM_TIMLEN_OFFSET_ID;
 uint16 M_BCNTRIM_RXMBSS_OFFSET_ID;
 uint16 M_BCNTRIM_TIMNOTFOUND_OFFSET_ID;
 uint16 M_BCNTRIM_CANTRIM_OFFSET_ID;
 uint16 M_BCNTRIM_LENCHG_OFFSET_ID;
 uint16 M_BCNTRIM_TSFDRF_OFFSET_ID;
 uint16 M_BCNTRIM_TIMBITSET_OFFSET_ID;
 uint16 M_BCNTRIM_WAKE_OFFSET_ID;
 uint16 M_BCNTRIM_SSID_OFFSET_ID;
 uint16 M_BCNTRIM_DTIM_OFFSET_ID;
 uint16 M_BCNTRIM_SSIDBLK_OFFSET_ID;
} d11shm_bcntrim_t;
typedef struct {
 uint16 M_PHY_NOISE_ID;
 uint16 M_RSSI_LOCK_OFFSET_ID;
 uint16 M_RSSI_LOGNSAMPS_OFFSET_ID;
 uint16 M_RSSI_NSAMPS_OFFSET_ID;
 uint16 M_RSSI_IQEST_EN_OFFSET_ID;
 uint16 M_RSSI_BOARDATTEN_DBG_OFFSET_ID;
 uint16 M_RSSI_IQPWR_DBG_OFFSET_ID;
 uint16 M_RSSI_IQPWR_DB_DBG_OFFSET_ID;
 uint16 M_NOISE_IQPWR_ID;
 uint16 M_NOISE_IQPWR_OFFSET_ID;
 uint16 M_NOISE_IQPWR_DB_OFFSET_ID;
 uint16 M_NOISE_LOGNSAMPS_ID;
 uint16 M_NOISE_LOGNSAMPS_OFFSET_ID;
 uint16 M_NOISE_NSAMPS_ID;
 uint16 M_NOISE_NSAMPS_OFFSET_ID;
 uint16 M_NOISE_IQEST_EN_OFFSET_ID;
 uint16 M_NOISE_IQEST_PENDING_ID;
 uint16 M_NOISE_IQEST_PENDING_OFFSET_ID;
 uint16 M_RSSI_IQEST_PENDING_OFFSET_ID;
 uint16 M_NOISE_LTE_ON_ID;
 uint16 M_NOISE_LTE_IQPWR_DB_OFFSET_ID;
 uint16 M_SSLPN_RSSI_0_OFFSET_ID;
 uint16 M_SSLPN_SNR_0_logchPowAccOut_OFFSET_ID;
 uint16 M_SSLPN_SNR_0_errAccOut_OFFSET_ID;
 uint16 M_SSLPN_RSSI_1_OFFSET_ID;
 uint16 M_SSLPN_SNR_1_logchPowAccOut_OFFSET_ID;
 uint16 M_SSLPN_SNR_1_errAccOut_OFFSET_ID;
 uint16 M_SSLPN_RSSI_2_OFFSET_ID;
 uint16 M_SSLPN_SNR_2_logchPowAccOut_OFFSET_ID;
 uint16 M_SSLPN_SNR_2_errAccOut_OFFSET_ID;
 uint16 M_SSLPN_RSSI_3_OFFSET_ID;
 uint16 M_SSLPN_SNR_3_logchPowAccOut_OFFSET_ID;
 uint16 M_SSLPN_SNR_3_errAccOut_OFFSET_ID;
 uint16 M_RSSI_QDB_0_OFFSET_ID;
 uint16 M_RSSI_QDB_1_OFFSET_ID;
 uint16 M_RSSI_QDB_2_OFFSET_ID;
 uint16 M_RSSI_QDB_3_OFFSET_ID;
} d11shm_rev_sslp_lt_40_t;
typedef struct {
 uint16 M_WOWL_TEST_CYCLE_ID;
 uint16 M_WOWL_TMR_L_ID;
 uint16 M_WOWL_TMR_ML_ID;
 uint16 M_KEYRC_LAST_ID;
 uint16 M_NETPAT_BLK_PTR_ID;
 uint16 M_WOWL_GPIOSEL_ID;
 uint16 M_NETPAT_NUM_ID;
 uint16 M_AESTABLES_PTR_ID;
 uint16 M_AID_NBIT_ID;
 uint16 M_HOST_WOWLBM_ID;
 uint16 M_GROUPKEY_UPBM_ID;
 uint16 M_WOWL_OFFLOADCFG_PTR_ID;
 uint16 M_CTX_GTKMSG2_ID;
 uint16 M_TXPHYERR_CNT_ID;
 uint16 M_SECSUITE_ID;
 uint16 M_TSCPN_BLK_ID;
 uint16 M_WAKEEVENT_IND_ID;
 uint16 M_EAPOLMICKEY_BLK_ID;
 uint16 M_RXFRM_SRA0_ID;
 uint16 M_RXFRM_SRA1_ID;
 uint16 M_RXFRM_SRA2_ID;
 uint16 M_RXFRM_RA0_ID;
 uint16 M_RXFRM_RA1_ID;
 uint16 M_RXFRM_RA2_ID;
 uint16 M_TXPSPFRM_CNT_ID;
 uint16 M_WOWL_NOBCN_ID;
} d11shm_wowl_t;
typedef struct {
 uint16 M_DRVR_UCODE_IF_PTR_ID;
 uint16 M_ULP_FEATURES_OFFSET_ID;
 uint16 M_DRIVER_BLOCK_OFFSET_ID;
 uint16 M_CRX_BLK_ID;
 uint16 M_RXFRM_BASE_ADDR_ID;
 uint16 M_SAVERESTORE_4335_BLK_ID;
 uint16 M_ILP_PER_H_OFFSET_ID;
 uint16 M_ILP_PER_L_OFFSET_ID;
 uint16 M_DRIVER_BLOCK_ID;
 uint16 M_FCBS_DS1_MAC_INIT_BLOCK_OFFSET_ID;
 uint16 M_FCBS_DS1_PHY_RADIO_BLOCK_OFFSET_ID;
 uint16 M_FCBS_DS1_RADIO_PD_BLOCK_OFFSET_ID;
 uint16 M_FCBS_DS1_EXIT_BLOCK_OFFSET_ID;
 uint16 M_FCBS_DS0_RADIO_PU_BLOCK_OFFSET_ID;
 uint16 M_FCBS_DS0_RADIO_PD_BLOCK_OFFSET_ID;
 uint16 M_ULP_WAKE_IND_ID;
 uint16 M_ILP_PER_H_ID;
 uint16 M_ILP_PER_L_ID;
 uint16 M_DS1_CTRL_SDIO_PIN_ID;
 uint16 M_DS1_CTRL_SDIO_ID;
 uint16 M_RXBEACONMBSS_CNT_ID;
 uint16 M_FRRUN_LBCN_CNT_ID;
 uint16 M_FCBS_DS0_RADIO_PU_BLOCK_ID;
 uint16 M_FCBS_DS0_RADIO_PD_BLOCK_ID;
} d11shm_bcmulp_t;
typedef struct {
 uint16 M_TS_SYNC_GPIO_ID;
 uint16 M_TS_SYNC_TSF_L_ID;
 uint16 M_TS_SYNC_TSF_ML_ID;
 uint16 M_TS_SYNC_AVB_L_ID;
 uint16 M_TS_SYNC_AVB_H_ID;
 uint16 M_TS_SYNC_PMU_L_ID;
 uint16 M_TS_SYNC_PMU_H_ID;
 uint16 M_TS_SYNC_TXTSF_ML_ID;
 uint16 M_TS_SYNC_GPIOMINDLY_ID;
 uint16 M_TS_SYNC_GPIOREALDLY_ID;
} d11shm_tsync_t;
typedef struct {
 uint16 M_OPS_MODE_ID;
 uint16 M_OPS_RSSI_THRSH_ID;
 uint16 M_OPS_MAX_LMT_ID;
 uint16 M_OPS_HIST_ID;
 uint16 M_OPS_LIGHT_L_ID;
 uint16 M_OPS_LIGHT_H_ID;
 uint16 M_OPS_FULL_L_ID;
 uint16 M_OPS_FULL_H_ID;
 uint16 M_OPS_NAV_CNT_ID;
 uint16 M_OPS_PLCP_CNT_ID;
 uint16 M_OPS_RSSI_CNT_ID;
 uint16 M_OPS_MISS_CNT_ID;
 uint16 M_OPS_MAXLMT_CNT_ID;
 uint16 M_OPS_MYBSS_CNT_ID;
 uint16 M_OPS_OBSS_CNT_ID;
 uint16 M_OPS_WAKE_CNT_ID;
 uint16 M_OPS_BCN_CNT_ID;
} d11shm_ops_t;
typedef struct {
 uint16 M_UCODE_MACSTAT_ID;
 uint16 M_UCODE_MACSTAT1_PTR_ID;
 uint16 M_SYNTHPU_DLY_ID;
 uint16 MX_PSM_SOFT_REGS_ID;
 uint16 MX_BOM_REV_MAJOR_ID;
 uint16 MX_BOM_REV_MINOR_ID;
 uint16 MX_UCODE_FEATURES_ID;
 uint16 MX_UCODE_DATE_ID;
 uint16 MX_UCODE_TIME_ID;
 uint16 MX_UCODE_DBGST_ID;
 uint16 MX_WATCHDOG_8TU_ID;
 uint16 MX_MACHW_VER_ID;
 uint16 MX_PHYVER_ID;
 uint16 MX_PHYTYPE_ID;
 uint16 MX_HOST_FLAGS0_ID;
 uint16 MX_HOST_FLAGS1_ID;
 uint16 MX_HOST_FLAGS2_ID;
 uint16 MX_BFI_BLK_ID;
 uint16 MX_NDPPWR_TBL_ID;
 uint16 MX_VMU_NDPPWR_TBL_ID;
 uint16 MX_HMU_NDPPWR_TBL_ID;
 uint16 MX_MUSND_PER_ID;
 uint16 MX_UTRACE_SPTR_ID;
 uint16 MX_UTRACE_EPTR_ID;
 uint16 M_AGGMPDU_HISTO_ID;
 uint16 M_AGGSTOP_HISTO_ID;
 uint16 M_MBURST_HISTO_ID;
 uint16 M_TXBCN_DUR_ID;
 uint16 M_PHYPREEMPT_VAL_ID;
} d11shm_rev_ge_64_t;
typedef struct {
 uint16 M_COREMASK_HETB_ID;
 uint16 M_RXTRIG_CMNINFO_ID;
 uint16 M_RXTRIG_USRINFO_ID;
 uint16 M_TXERR_PHYSTS_ID;
 uint16 M_TXERR_REASON0_ID;
 uint16 M_TXERR_REASON1_ID;
 uint16 M_TXERR_TXDUR_ID;
 uint16 M_TXERR_PCTLEN_ID;
 uint16 M_TXERR_PCTL4_ID;
 uint16 M_TXERR_PCTL9_ID;
 uint16 M_TXERR_PCTL10_ID;
 uint16 M_TXERR_CCLEN_ID;
 uint16 M_TXERR_TXBYTES_L_ID;
 uint16 M_TXERR_TXBYTES_H_ID;
 uint16 M_TXERR_UNFLSTS_ID;
 uint16 M_TXERR_USR_ID;
 uint16 M_RXTRIG_MYAID_CNT_ID;
 uint16 M_RXTRIG_RAND_CNT_ID;
 uint16 M_RXSWRST_CNT_ID;
 uint16 M_RXSFCQI_CNT_ID;
 uint16 M_NDPAUSR_CNT_ID;
 uint16 M_BFD_DONE_CNT_ID;
 uint16 M_BFD_FAIL_CNT_ID;
 uint16 M_RXSFERR_CNT_ID;
 uint16 M_RXPFFLUSH_CNT_ID;
 uint16 M_RXFLUCMT_CNT_ID;
 uint16 M_RXFLUOV_CNT_ID;
 uint16 MX_HEMSCH_BLKS_ID;
 uint16 MX_HEMSCH0_BLK_ID;
 uint16 MX_HEMSCH0_SIGA_ID;
 uint16 MX_HEMSCH0_PCTL0_ID;
 uint16 MX_HEMSCH0_N_ID;
 uint16 MX_HEMSCH0_USR_ID;
 uint16 MX_HEMSCH0_URDY0_ID;
 uint16 M_TXTRIG_FLAG_ID;
 uint16 M_TXTRIG_NUM_ID;
 uint16 M_TXTRIG_LEN_ID;
 uint16 M_TXTRIG_RATE_ID;
 uint16 M_MUAGG_HISTO_ID;
 uint16 M_HEMMUAGG_HISTO_ID;
 uint16 M_HEOMUAGG_HISTO_ID;
 uint16 M_TXAMPDUSU_CNT_ID;
 uint16 M_TXTRIG_MINTIME_ID;
 uint16 M_TXTRIG_FRAME_ID;
 uint16 M_TXTRIG_CMNINFO_ID;
 uint16 M_ULRXCTL_BLK_ID;
 uint16 M_RTS_MINLEN_L_ID;
 uint16 M_RTS_MINLEN_H_ID;
 uint16 M_AGG0_CNT_ID;
 uint16 M_TRIGREFILL_CNT_ID;
 uint16 M_TXTRIG_CNT_ID;
 uint16 M_RXHETBBA_CNT_ID;
 uint16 M_TXBAMTID_CNT_ID;
 uint16 M_TXBAMSTA_CNT_ID;
 uint16 M_TXMBA_CNT_ID;
 uint16 M_RXMYDTINRSP_ID;
 uint16 M_RXEXIT_CNT_ID;
 uint16 M_PHYRXSFULL_CNT_ID;
 uint16 M_BFECAP_HE_ID;
 uint16 M_BFECAP_VHT_ID;
 uint16 M_BFECAP_HT_ID;
 uint16 M_BSS_BLK_ID;
 uint16 MX_MUBFI_BLK_ID;
 uint16 M_TXTRIG_SRXCTL_ID;
 uint16 M_TXTRIG_SRXCTLUSR_ID;
 uint16 M_ULTX_STS_ID;
 uint16 M_ULTX_ACMASK_ID;
 uint16 M_BCN_TXPCTL6_ID;
 uint16 MX_OQEXPECTN_BLK_ID;
 uint16 MX_OQMAXN_BLK_ID;
 uint16 MX_OMSCH_TMOUT_ID;
 uint16 MX_SNDREQ_BLK_ID;
 uint16 M_BFI_GENCFG_ID;
 uint16 MX_TRIG_TXCFG_ID;
 uint16 MX_TRIG_TXLMT_ID;
 uint16 M_PHYREG_TDSFO_VAL_ID;
 uint16 M_PHYREG_HWOBSS_VAL_ID;
 uint16 M_PHYREG_TX_SHAPER_COMMON12_VAL_ID;
 uint16 M_BSS_BSRT_BLK_ID;
 uint16 M_STXVM_BLK_ID;
 uint16 MX_TXVBMP_BLK_ID;
 uint16 MX_MRQ_UPDPEND_ID;
 uint16 MX_TXVM_BLK_ID;
 uint16 MX_CQIBMP_BLK_ID;
 uint16 MX_CQIM_BLK_ID;
 uint16 M_TXBRPTRIG_CNT_ID;
 uint16 MX_MACREQ_BLK_ID;
 uint16 M_TXVMSTATS_BLK_ID;
 uint16 M_CQIMSTATS_BLK_ID;
 uint16 M_TXVFULL_CNT_ID;
 uint16 MX_SNDAGE_THRSH_ID;
 uint16 M_HETB_CSTHRSH_LO_ID;
 uint16 M_HETB_CSTHRSH_HI_ID;
 uint16 MX_CURCHANNEL_ID;
 uint16 MX_OQMINN_BLK_ID;
 uint16 M_D11SR_BLK_ID;
 uint16 M_D11SR_NSRG_PDMIN_ID;
 uint16 M_D11SR_NSRG_PDMAX_ID;
 uint16 M_D11SR_SRG_PDMIN_ID;
 uint16 M_D11SR_SRG_PDMAX_ID;
 uint16 M_D11SR_TXPWRREF_ID;
 uint16 M_D11SR_NSRG_TXPWRREF0_ID;
 uint16 M_D11SR_SRG_TXPWRREF0_ID;
 uint16 M_D11SR_OPTIONS_ID;
 uint16 M_D11SROPP_CNT_ID;
 uint16 M_D11SRTX_CNT_ID;
 uint16 M_TWTCMD_ID;
 uint16 M_TWTINT_DATA_ID;
 uint16 M_PRETWT_US_ID;
 uint16 M_TWT_PRESTRT_ID;
 uint16 M_TWT_PRESTOP_ID;
 uint16 MX_TWT_PRESTRT_ID;
 uint16 MX_ULOMAXN_BLK_ID;
 uint16 M_TXTRIGWT0_VAL_ID;
 uint16 M_TXTRIGWT1_VAL_ID;
 uint16 MX_ULC_NUM_ID;
 uint16 MX_M2VMSG_CNT_ID;
 uint16 MX_V2MMSG_CNT_ID;
 uint16 MX_M2VGRP_CNT_ID;
 uint16 MX_V2MGRP_CNT_ID;
 uint16 MX_V2MGRPINV_CNT_ID;
 uint16 MX_M2VSND_CNT_ID;
 uint16 MX_V2MSND_CNT_ID;
 uint16 MX_V2MSNDINV_CNT_ID;
 uint16 MX_M2VCQI_CNT_ID;
 uint16 MX_V2MCQI_CNT_ID;
 uint16 MX_FFQADD_CNT_ID;
 uint16 MX_FFQDEL_CNT_ID;
 uint16 MX_MFQADD_CNT_ID;
 uint16 MX_MFQDEL_CNT_ID;
 uint16 MX_MFOQADD_CNT_ID;
 uint16 MX_MFOQDEL_CNT_ID;
 uint16 MX_OFQADD_CNT_ID;
 uint16 MX_OFQDEL_CNT_ID;
 uint16 MX_RUCFG_CNT_ID;
 uint16 MX_M2VRU_CNT_ID;
 uint16 MX_V2MRU_CNT_ID;
 uint16 MX_OFQAGG0_CNT_ID;
 uint16 MX_ULO_QNULLTHRSH_ID;
 uint16 MX_MACREQ_CNT_ID;
 uint16 MX_SNDFL_CNT_ID;
 uint16 MX_M2SQ0_CNT_ID;
 uint16 MX_M2SQ1_CNT_ID;
 uint16 MX_M2SQ2_CNT_ID;
 uint16 MX_M2SQ3_CNT_ID;
 uint16 MX_M2SQ4_CNT_ID;
 uint16 MX_HEMUCAPINV_CNT_ID;
 uint16 MX_M2SQTXVEVT_CNT_ID;
 uint16 MX_MMUREGRP_CNT_ID;
 uint16 MX_MMUEMGCGRP_CNT_ID;
 uint16 MX_M2VGRPWAIT_CNT_ID;
 uint16 MX_OM2SQ0_CNT_ID;
 uint16 MX_OM2SQ1_CNT_ID;
 uint16 MX_OM2SQ2_CNT_ID;
 uint16 MX_OM2SQ3_CNT_ID;
 uint16 MX_OM2SQ4_CNT_ID;
 uint16 MX_OM2SQ5_CNT_ID;
 uint16 MX_OM2SQ6_CNT_ID;
 uint16 MX_TAFWMISMATCH_CNT_ID;
 uint16 MX_OMREDIST_CNT_ID;
 uint16 MX_OMUTXDLMEMTCH_CNT_ID;
 uint16 M_CSI_STATUS_ID;
 uint16 M_CSI_BLKS_ID;
 uint16 M_TXMURTS_CNT_ID;
 uint16 M_TXTRIGAMPDU_CNT_ID;
 uint16 M_TXMUBAR_CNT_ID;
 uint16 M_BSS_BCNPRS_PWR_BLK_ID;
 uint16 M_PPR_BLK_ID;
 uint16 M_MAX_TXPWR_ID;
 uint16 M_BRDLMT_BLK_ID;
 uint16 M_RULMT_BLK_ID;
 uint16 MX_SCHED_HIST_ID;
 uint16 MX_SCHED_MAX_ID;
 uint16 M_PSMWDS_PC_ID;
 uint16 MX_PSMWDS_PC_ID;
 uint16 M_TXQNL_CNT_ID;
 uint16 M_HETBFP_CNT_ID;
 uint16 M_TXHETBBA_CNT_ID;
 uint16 MX_ULOFIFO_BASE_ID;
 uint16 MX_ULOFIFO_BMP_ID;
 uint16 M_NAV_MAX_VAL_ID;
 uint16 M_RPTOVRD_CNT_ID;
 uint16 MX_GRP_HIST_BLK_ID;
 uint16 MX_FFQ_GAP_BLK_ID;
 uint16 M_ULTX_HOLDTM_L_ID;
 uint16 M_ULTX_HOLDTM_H_ID;
 uint16 M_CUR_BSSCOLOR_ID;
 uint16 M_MY_BSSCOLOR_ID;
 uint16 M_COCLS_CNT_ID;
 uint16 M_SKIP_FFQCONS_CNT_ID;
 uint16 MX_AGGX_SUTHRSH_ID;
 uint16 MX_AGGX_MUTHRSH_ID;
 uint16 M_RSSICOR_BLK_ID;
} d11shm_rev_ge_128_t;
typedef struct {
 uint16 M_DTIM_TXENQD_BMBP_ID;
 uint16 M_DTIMCNT_MBSS_BMBP_ID;
 uint16 M_ODAP_ACK_TMOUT_ID;
 uint16 M_ODAP_ACK_TMOUT_TEST_ID;
 uint16 M_MBS_DAGGOFF_BMP_ID;
 uint16 M_CTRL_CHANNEL_ID;
 uint16 M_ENT_HOST_FLAGS1_ID;
 uint16 M_EAP_BLK_0_ID;
 uint16 M_EAP_BLK_1_ID;
 uint16 M_UCODE_CAP_L_ID;
 uint16 M_UCODE_CAP_H_ID;
 uint16 M_UCODE_FEATURE_EN_L_ID;
 uint16 M_UCODE_FEATURE_EN_H_ID;
 uint16 M_PRS_RETRY_THR_ID;
 uint16 M_MBSS_CCK_BITMAP_ID;
 uint16 M_GEN_DBG0_ID;
 uint16 M_EAP_BLK_2_ID;
 uint16 M_SAS_DEBUG_BLK_ID;
 uint16 M_EAPSC1_SUCC_CNT_ID;
 uint16 M_EAPSC1_FAIL_CNT_ID;
 uint16 M_EAPSP1_SUCC_CNT_ID;
 uint16 M_EAPSP1_FAIL_CNT_ID;
 uint16 M_EAPFC_SUCC_CNT_ID;
 uint16 M_EAPFC_FAIL_CNT_ID;
 uint16 M_EAPFC1_SUCC_CNT_ID;
 uint16 M_EAPFC1_FAIL_CNT_ID;
 uint16 M_SAS_RESP_AI_BLK_ID;
 uint16 M_DTIM_MBSS_BLK_ID;
 uint16 M_TIM_OFF_MBSS_BLK_ID;
 uint16 M_SAS_RESP_AI_IDX_ID;
 uint16 M_SAS_RESP_AI_BLK_PTR_ID;
 uint16 M_SAS_DBG_4_ID;
 uint16 M_SAS_DBG_5_ID;
 uint16 M_SASAIFIFO_OFLO_CNT_ID;
 uint16 M_TXQ_PRUNE_TABLE_ID;
 uint16 M_FFT_SMPL_FFT_GAIN_RX0_ID;
 uint16 M_FFT_SMPL_FFT_GAIN_RX1_ID;
 uint16 M_FFT_SMPL_FFT_GAIN_RX2_ID;
 uint16 M_FFT_SMPL_CTRL_ID;
 uint16 M_FFT_SMPL_TS_ID;
 uint16 M_FFT_SMPL_FRMCNT_ID;
 uint16 M_FFT_SMPL_RX_CHAIN_ID;
 uint16 M_SASAIFIFO_RPTR_ID;
 uint16 M_SASAIFIFO_WPTR_ID;
 uint16 M_FFT_SMPL_WLAN_GAIN_RX0_ID;
 uint16 M_FFT_SMPL_WLAN_GAIN_RX1_ID;
 uint16 M_FFT_SMPL_WLAN_GAIN_RX2_ID;
 uint16 M_FFT_SMPL_STATUS_ID;
 uint16 M_FFT_SMPL_SEQUENCE_NUM_ID;
 uint16 M_FFT_CRS_GLITCH_CNT_ID;
 uint16 M_FAST_NOISE_TS_ID;
 uint16 M_FAST_NOISE_DIFF_ID;
 uint16 M_SAS_FRM_RX_AI_ID;
 uint16 M_SAS_IMPBF_AI_ID;
 uint16 M_FAST_NOISE_FORCE_ID;
 uint16 M_FFT_SMPL_CHANNEL_ID;
 uint16 M_FFT_SMPL_FFT_GAIN_RX3_ID;
 uint16 M_FAST_NOISE_MEASURE_ID;
 uint16 M_FAST_NOISE_DUR_ID;
 uint16 M_SAS_DEFAULT_AI_RXD_ID;
 uint16 M_SAS_DEFAULT_AI_TXD_ID;
 uint16 M_SAS_GPIO_CLK_ID;
 uint16 M_SAS_GPIO_DATA_ID;
 uint16 M_FFT_SMPL_ENABLE_ID;
 uint16 M_FFT_SMPL_INTERVAL_ID;
 uint16 M_EAP_BLK_RESERVED_1_ID;
 uint16 M_EAP_BLK_RESERVED_2_ID;
 uint16 M_FIPS_LPB_ENC_BLK_ID;
 uint16 M_FIPS_LPB_MIC_BLK_ID;
 uint16 M_NM_ATTEMPTS_ID;
 uint16 M_NM_IFS_START_ID;
 uint16 M_NM_IFS_END_ID;
 uint16 M_NM_EDCRS_ID;
 uint16 M_NM_PKTPROC_ID;
 uint16 M_NM_POST_CLEAN_ID;
 uint16 M_NM_PKTPROC2_ID;
 uint16 M_NM_IFS_MID_ID;
 uint16 M_DIAG_FLAGS_ID;
} d11shm_eap_t;
typedef struct shmdefs_struct {
 const d11shm_common_base_t *common_base;
 const d11shm_proxd_t *proxd;
 const d11shm_mcnx_t *mcnx;
 const d11shm_p2p_t *p2p;
 const d11shm_mfgtest_t *mfgtest;
 const d11shm_km_t *km;
 const d11shm_txbf_t *txbf;
 const d11shm_macstat_t *macstat;
 const d11shm_mbssid_t *mbssid;
 const d11shm_physim_t *physim;
 const d11shm_ulpphysim_t *ulpphysim;
 const d11shm_btcx_t *btcx;
 const d11shm_ltecx_t *ltecx;
 const d11shm_ccastat_t *ccastat;
 const d11shm_ampdu_t *ampdu;
 const d11shm_btamp_t *btamp;
 const d11shm_rev_ge_40_t *rev_ge_40;
 const d11shm_rev_lt_40_t *rev_lt_40;
 const d11shm_bcntrim_t *bcntrim;
 const d11shm_rev_sslp_lt_40_t *rev_sslp_lt_40;
 const d11shm_wowl_t *wowl;
 const d11shm_bcmulp_t *bcmulp;
 const d11shm_tsync_t *tsync;
 const d11shm_ops_t *ops;
 const d11shm_rev_ge_64_t *rev_ge_64;
 const d11shm_rev_ge_128_t *rev_ge_128;
 const d11shm_eap_t *eap;
} shmdefs_t;
static const d11shm_common_base_t common_base_ucode_std_132 = {
 .M_SSID_ID = (0xb0*2),
 .M_PRS_FRM_BYTESZ_ID = ((0x0*2)+(0x25*2)),
 .M_SSID_BYTESZ_ID = ((0x0*2)+(0x24*2)),
 .M_PMQADDINT_THSD_ID = ((0x0*2)+(0x15*2)),
 .M_RT_BBRSMAP_A_ID = (0xf0*2),
 .M_RT_BBRSMAP_B_ID = (0x110*2),
 .M_FIFOSIZE0_ID = INVALID,
 .M_FIFOSIZE1_ID = INVALID,
 .M_FIFOSIZE2_ID = INVALID,
 .M_FIFOSIZE3_ID = INVALID,
 .M_MBURST_TXOP_ID = ((0x0*2)+(0x41*2)),
 .M_MBURST_SIZE_ID = ((0x0*2)+(0x40*2)),
 .M_MODE_CORE_ID = ((0xc0*2)+(0xe*2)),
 .M_WLCX_CONFIG_ID = INVALID,
 .M_WLCX_GPIO_CONFIG_ID = INVALID,
 .M_DOT11_DTIMPERIOD_ID = ((0x0*2)+(0x9*2)),
 .M_TIMBC_OFFSET_ID = ((0x0*2)+(0x65*2)),
 .M_CCKBOOST_ADJ_ID = INVALID,
 .M_EDCF_BLKS_ID = (0x120*2),
 .M_EDCF_QINFO1_ID = ((0x120*2)+(0x10*2)),
 .M_EDCF_QINFO2_ID = ((0x120*2)+(0x20*2)),
 .M_EDCF_QINFO3_ID = ((0x120*2)+(0x30*2)),
 .M_EDCF_BOFF_CTR_ID = ((0x120*2)+(0x4f*2)),
 .M_LNA_ROUT_ID = ((0x120*2)+(0x52*2)),
 .M_RXGAIN_HI_ID = ((0x120*2)+(0x59*2)),
 .M_RXGAIN_LO_ID = ((0x120*2)+(0x5a*2)),
 .M_RXGAINOVR2_VAL_ID = ((0x120*2)+(0x58*2)),
 .M_LPFGAIN_HI_ID = ((0x120*2)+(0x5b*2)),
 .M_LPFGAIN_LO_ID = ((0x120*2)+(0x5c*2)),
 .M_CUR_TXF_INDEX_ID = INVALID,
 .M_COREMASK_BLK_ID = (0x1ca*2),
 .M_COREMASK_BPHY_ID = ((0x1ca*2)+(0x0*2)),
 .M_COREMASK_BFM_ID = ((0x1ca*2)+(0x2*2)),
 .M_COREMASK_BFM1_ID = ((0x1ca*2)+(0x3*2)),
 .M_COREMASK_BTRESP_ID = ((0x1ca*2)+(0x5*2)),
 .M_TXPWR_BLK_ID = (0x1d4*2),
 .M_PS_MORE_DTIM_TBTT_ID = ((0x0*2)+(0x5b*2)),
 .M_POSTDTIM0_NOSLPTIME_ID = ((0x0*2)+(0x26*2)),
 .M_BCMC_FID_ID = ((0x0*2)+(0x54*2)),
 .M_AMT_INFO_PTR_ID = ((0x0*2)+(0x17*2)),
 .M_PRS_MAXTIME_ID = ((0x0*2)+(0x3a*2)),
 .M_PRETBTT_ID = ((0x0*2)+(0x4b*2)),
 .M_BCN_TXTSF_OFFSET_ID = ((0x0*2)+(0xe*2)),
 .M_TIMBPOS_INBEACON_ID = ((0x0*2)+(0xf*2)),
 .M_AGING_THRSH_ID = ((0x0*2)+(0x3e*2)),
 .M_SYNTHPU_DELAY_ID = ((0x0*2)+(0x4a*2)),
 .M_PHYVER_ID = ((0x0*2)+(0x28*2)),
 .M_PHYTYPE_ID = ((0x0*2)+(0x29*2)),
 .M_MAX_ANTCNT_ID = ((0x0*2)+(0x2e*2)),
 .M_MACHW_VER_ID = ((0x0*2)+(0xb*2)),
 .M_MACHW_CAP_L_ID = ((0x0*2)+(0x60*2)),
 .M_MACHW_CAP_H_ID = ((0x0*2)+(0x61*2)),
 .M_SFRMTXCNTFBRTHSD_ID = ((0x0*2)+(0x22*2)),
 .M_LFRMTXCNTFBRTHSD_ID = ((0x0*2)+(0x23*2)),
 .M_TXDC_BYTESZ_ID = INVALID,
 .M_TXFL_BMAP_ID = ((0x0*2)+(0x3f*2)),
 .M_JSSI_AUX_ID = ((0x0*2)+(0x46*2)),
 .M_DOT11_SLOT_ID = ((0x0*2)+(0x8*2)),
 .M_DUTY_STRTRATE_ID = ((0x0*2)+(0x6b*2)),
 .M_SECRSSI0_MIN_ID = ((0xc0*2)+(0x5*2)),
 .M_SECRSSI1_MIN_ID = ((0xc0*2)+(0x6*2)),
 .M_MIMO_ANTSEL_RXDFLT_ID = INVALID,
 .M_MIMO_ANTSEL_TXDFLT_ID = INVALID,
 .M_ANTSEL_CLKDIV_ID = INVALID,
 .M_BOM_REV_MAJOR_ID = ((0x0*2)+(0x0*2)),
 .M_BOM_REV_MINOR_ID = ((0x0*2)+(0x1*2)),
 .M_HOST_FLAGS_ID = ((0x0*2)+(0x2f*2)),
 .M_HOST_FLAGS2_ID = ((0x0*2)+(0x30*2)),
 .M_HOST_FLAGS3_ID = ((0x0*2)+(0x31*2)),
 .M_HOST_FLAGS4_ID = ((0x0*2)+(0x3c*2)),
 .M_HOST_FLAGS5_ID = ((0x0*2)+(0x6a*2)),
 .M_HOST_FLAGS6_ID = ((0xc0*2)+(0x3*2)),
 .M_REV_L_ID = ((0x0*2)+(0x2*2)),
 .M_REV_H_ID = ((0x0*2)+(0x3*2)),
 .M_UCODE_FEATURES_ID = ((0x0*2)+(0x5*2)),
 .M_ULP_STATUS_ID = INVALID,
 .M_EDCF_QINFO1_OFFSET_ID = (0x10*2),
 .M_MYMAC_ADDR_ID = (0x1c4*2),
 .M_UTRACE_SPTR_ID = ((0x0*2)+(0x38*2)),
 .M_UTRACE_EPTR_ID = ((0x0*2)+(0x39*2)),
 .M_UTRACE_STS_ID = (0x1423*2),
 .M_TXFRAME_CNT_ID = ((0x70*2)+(0x0*2)),
 .M_TXAMPDU_CNT_ID = ((0x70*2)+(0xc*2)),
 .M_RXSTRT_CNT_ID = ((0x70*2)+(0x18*2)),
 .M_RXCRSGLITCH_CNT_ID = ((0x70*2)+(0x17*2)),
 .M_BPHYGLITCH_CNT_ID = ((0x70*2)+(0x3c*2)),
 .M_RXBADFCS_CNT_ID = ((0x70*2)+(0x15*2)),
 .M_RXBADPLCP_CNT_ID = ((0x70*2)+(0x16*2)),
 .M_RXBPHY_BADPLCP_CNT_ID = ((0x70*2)+(0x3f*2)),
 .M_AC_TXLMT_BLK_ID = (0x180*2),
 .M_MAXRXFRM_LEN_ID = ((0x0*2)+(0x10*2)),
 .M_MAXRXFRM_LEN2_ID = ((0x0*2)+(0x19*2)),
 .M_MAXRXMPDU_LEN_ID = ((0x0*2)+(0x11*2)),
 .M_RXCORE_STATE_ID = (0x1150*2),
 .M_ASSERT_REASON_ID = ((0xc0*2)+(0x1b*2)),
 .M_WRDCNT_RXF1OVFL_THRSH_ID = INVALID,
 .M_WRDCNT_RXF0OVFL_THRSH_ID = INVALID,
 .M_NCAL_ACTIVE_CORES_ID = INVALID,
 .M_NCAL_CFG_BMP_ID = INVALID,
 .M_NCAL_RFCTRLOVR_0_ID = INVALID,
 .M_NCAL_RXGAINOVR_0_ID = INVALID,
 .M_NCAL_RXLPFOVR_0_ID = INVALID,
 .M_NCAL_RXGAIN_LO_ID = INVALID,
 .M_NCAL_LPFGAIN_LO_ID = INVALID,
 .M_NCAL_RXGAIN_HI_ID = INVALID,
 .M_NCAL_LPFGAIN_HI_ID = INVALID,
 .M_NCAL_RFCTRLOVR_VAL_ID = INVALID,
 .M_BSR_BLK_ID = INVALID,
 .M_SEMAPHORE_BSR_ID = INVALID,
 .M_BSR_ALLTID_SET0_ID = INVALID,
 .M_BSR_ALLTID_SET1_ID = INVALID,
 .M_ACKPWRTBL_R0_1_ID = INVALID,
 .M_ACKPWRTBL_R0_2_ID = INVALID,
 .M_ACKPWRTBL_R1_0_ID = INVALID,
 .M_ACKPWRTBL_R1_1_ID = INVALID,
 .M_ACKPWRTBL_R2_0_ID = INVALID,
 .M_ACKPWRTBL_R2_1_ID = INVALID,
 .M_ACKPWRTBL_R3_0_ID = INVALID,
 .M_ACKPWRTBL_R3_1_ID = INVALID,
 .M_ACKPWRTBL_R4_0_ID = INVALID,
 .M_ACKPWRTBL_R4_1_ID = INVALID,
 .M_ACKPWRTBL_R5_0_ID = INVALID,
 .M_ACKPWRTBL_R5_1_ID = INVALID,
 .M_ACKPWRTBL_R6_0_ID = INVALID,
 .M_ACKPWRTBL_R6_1_ID = INVALID,
 .M_ACKPWRTBL_R7_0_ID = INVALID,
 .M_ACKPWRTBL_R7_1_ID = INVALID,
 .M_ACKPWRTBL_R8_0_ID = INVALID,
 .M_ACKPWRTBL_R8_1_ID = INVALID,
 .M_ACK_2GCM_NUMB_ID = INVALID,
 .M_BTACK_2GCM_NUMB_ID = INVALID,
 .M_CUR_ACK_OFST_ID = INVALID,
 .M_TXPWRCAP_C0_ID = INVALID,
 .M_TXPWRCAP_C1_ID = INVALID,
 .M_TXPWRCAP_C2_ID = INVALID,
 .M_TXPWRCAP_C0_FS_ID = INVALID,
 .M_TXPWRCAP_C1_FS_ID = INVALID,
 .M_TXPWRCAP_C2_FS_ID = INVALID,
 .M_TXPWRCAP_C0_FSANT_ID = INVALID,
 .M_TXPWRCAP_BT_C0_ID = INVALID,
 .M_TXPWRCAP_BT_C1_ID = INVALID,
 .M_TXPWRCAP_BT_C2_ID = INVALID,
 .M_TDMTX_RSSI_THR_ID = INVALID,
 .M_TDMTX_TXPWRBOFF_ID = INVALID,
 .M_TDMTX_TXPWRBOFF_DT_ID = INVALID,
 .M_CCA_MEASINTV_L_ID = ((0xa9e*2)+(0x0*2)),
 .M_CCA_MEASINTV_H_ID = ((0xa9e*2)+(0x1*2)),
 .M_CCA_MEASBUSY_L_ID = ((0xa9e*2)+(0x2*2)),
 .M_CCA_MEASBUSY_H_ID = ((0xa9e*2)+(0x3*2)),
};
static const d11shm_proxd_t proxd_ucode_std_132 = {
 .M_TOF_BLK_PTR_ID = ((0x0*2)+(0x45*2)),
 .M_TOF_CMD_OFFSET_ID = (0x0*2),
 .M_TOF_RSP_OFFSET_ID = (0x1*2),
 .M_TOF_CHNSM_0_OFFSET_ID = (0x2*2),
 .M_TOF_DOT11DUR_OFFSET_ID = (0x3*2),
 .M_TOF_PHYCTL0_OFFSET_ID = (0x4*2),
 .M_TOF_PHYCTL1_OFFSET_ID = (0x5*2),
 .M_TOF_PHYCTL2_OFFSET_ID = (0x6*2),
 .M_TOF_PHYCTL0_ID = ((0x10ba*2)+(0x4*2)),
 .M_TOF_PHYCTL1_ID = ((0x10ba*2)+(0x5*2)),
 .M_TOF_PHYCTL2_ID = ((0x10ba*2)+(0x6*2)),
 .M_TOF_HT_PHYCTL0_ID = ((0x10ba*2)+(0xa*2)),
 .M_TOF_HT_PHYCTL1_ID = ((0x10ba*2)+(0xb*2)),
 .M_TOF_HT_PHYCTL2_ID = ((0x10ba*2)+(0xc*2)),
 .M_TOF_HE_PHYCTL0_ID = ((0x10ba*2)+(0x10*2)),
 .M_TOF_HE_PHYCTL1_ID = ((0x10ba*2)+(0x11*2)),
 .M_TOF_HE_PHYCTL2_ID = ((0x10ba*2)+(0x12*2)),
 .M_TOF_LSIG_OFFSET_ID = INVALID,
 .M_TOF_VHTA0_OFFSET_ID = (0x7*2),
 .M_TOF_VHTA1_OFFSET_ID = (0x8*2),
 .M_TOF_VHTA2_OFFSET_ID = (0x9*2),
 .M_TOF_VHTA0_ID = ((0x10ba*2)+(0x7*2)),
 .M_TOF_VHTA1_ID = ((0x10ba*2)+(0x8*2)),
 .M_TOF_VHTA2_ID = ((0x10ba*2)+(0x9*2)),
 .M_TOF_VHTB0_OFFSET_ID = INVALID,
 .M_TOF_VHTB1_OFFSET_ID = INVALID,
 .M_TOF_HTSIG0_ID = ((0x10ba*2)+(0xd*2)),
 .M_TOF_HTSIG1_ID = ((0x10ba*2)+(0xe*2)),
 .M_TOF_HTSIG2_ID = ((0x10ba*2)+(0xf*2)),
 .M_TOF_HESIG0_ID = ((0x10ba*2)+(0x13*2)),
 .M_TOF_HESIG1_ID = ((0x10ba*2)+(0x14*2)),
 .M_TOF_HESIG2_ID = ((0x10ba*2)+(0x15*2)),
 .M_TOF_FLAGS_OFFSET_ID = (0x3e*2),
 .M_TOF_FLAGS_ID = ((0x10ba*2)+(0x3e*2)),
 .M_TOF_AVB_BLK_ID = ((0x10ba*2)+(0x44*2)),
 .M_TOF_AVB_BLK_IDX_ID = ((0x10ba*2)+(0x50*2)),
 .M_TOF_DBG_BLK_OFFSET_ID = INVALID,
 .M_TOF_DBG1_OFFSET_ID = INVALID,
 .M_TOF_DBG2_OFFSET_ID = INVALID,
 .M_TOF_DBG3_OFFSET_ID = INVALID,
 .M_TOF_DBG4_OFFSET_ID = INVALID,
 .M_TOF_DBG5_OFFSET_ID = INVALID,
 .M_TOF_DBG6_OFFSET_ID = INVALID,
 .M_TOF_DBG7_OFFSET_ID = INVALID,
 .M_TOF_DBG8_OFFSET_ID = INVALID,
 .M_TOF_DBG9_OFFSET_ID = INVALID,
 .M_TOF_DBG10_OFFSET_ID = INVALID,
 .M_TOF_DBG11_OFFSET_ID = INVALID,
 .M_TOF_DBG12_OFFSET_ID = INVALID,
 .M_TOF_DBG13_OFFSET_ID = INVALID,
 .M_TOF_DBG14_OFFSET_ID = INVALID,
 .M_TOF_DBG15_OFFSET_ID = INVALID,
 .M_TOF_DBG16_OFFSET_ID = INVALID,
 .M_TOF_DBG17_OFFSET_ID = INVALID,
 .M_TOF_DBG18_OFFSET_ID = INVALID,
 .M_TOF_DBG19_OFFSET_ID = INVALID,
 .M_TOF_DBG20_OFFSET_ID = INVALID,
 .M_FTM_SC_CURPTR_ID = (0x1eda*2),
 .M_FTM_NYSM0_ID = (0x1edb*2),
};
static const d11shm_mcnx_t mcnx_ucode_std_132 = {
 .M_P2P_BLK_PTR_ID = ((0x0*2)+(0x57*2)),
 .M_P2P_PERBSS_BLK_ID = ((0x5ee*2)+(0x1d*2)),
 .M_P2P_INTF_BLK_ID = (0x5ee*2),
 .M_ADDR_BMP_BLK_OFFSET_ID = INVALID,
 .M_P2P_INTR_BLK_ID = ((0x5ee*2)+(0x0*2)),
 .M_P2P_HPS_ID = ((0x5ee*2)+(0x10*2)),
 .M_P2P_HPS_OFFSET_ID = (0x10*2),
 .M_P2P_TSF_OFFSET_BLK_ID = ((0x5ee*2)+(0x4d*2)),
 .M_P2P_GO_CHANNEL_ID = ((0x5ee*2)+(0x5d*2)),
 .M_P2P_GO_IND_BMP_ID = ((0x5ee*2)+(0x5e*2)),
 .M_P2P_PRETBTT_BLK_ID = ((0x5ee*2)+(0x5f*2)),
 .M_P2P_TSF_DRIFT_WD0_ID = (((0x5ee*2)+(0x63*2))+(0x0*2)),
 .M_P2P_TXSTOP_T_BLK_ID = ((0x5ee*2)+(0x67*2)),
};
static const d11shm_p2p_t p2p_ucode_std_132 = {
 .M_SHM_BYT_CNT_ID = ((0x0*2)+(0xa*2)),
};
static const d11shm_mfgtest_t mfgtest_ucode_std_132 = {
 .M_MFGTEST_FRMCNT_LO_ID = ((0x0*2)+(0x6e*2)),
 .M_MFGTEST_FRMCNT_HI_ID = ((0x0*2)+(0x6f*2)),
 .M_MFGTEST_NUM_ID = ((0x0*2)+(0x6c*2)),
 .M_MFGTEST_IO1_ID = ((0x0*2)+(0x6d*2)),
 .M_RXSTATS_BLK_PTR_ID = INVALID,
 .M_MFGTEST_UOTARXTEST_ID = INVALID,
 .M_MFGTEST_RXAVGPWR_ANT0_ID = INVALID,
 .M_MFGTEST_RXAVGPWR_ANT1_ID = INVALID,
 .M_MFGTEST_RXAVGPWR_ANT2_ID = INVALID,
};
static const d11shm_km_t km_ucode_std_132 = {
 .M_TKMICKEYS_PTR_ID = INVALID,
 .M_WAPIMICKEYS_PTR_ID = INVALID,
 .M_SECKEYS_PTR_ID = INVALID,
 .M_SECKINDX_PTR_ID = ((0x0*2)+(0x18*2)),
 .M_TKIP_TTAK_PTR_ID = ((0x0*2)+(0x1a*2)),
 .M_SECKINDXALGO_BLK_ID = (0xca4*2),
 .M_TKIP_TSC_TTAK_ID = (0xda8*2),
};
static const d11shm_txbf_t txbf_ucode_std_132 = {
 .M_BFI_BLK_ID = INVALID,
 .M_BFI_REFRESH_THR_ID = ((0x21a*2)+(0x1*2)),
 .M_BFI_NRXC_ID = ((0x21a*2)+(0x3*2)),
};
static const d11shm_macstat_t macstat_ucode_std_132 = {
 .M_PSM2HOST_STATS_ID = (0x70*2),
 .M_PSM2HOST_EXT_PTR_ID = ((0x0*2)+(0x1c*2)),
 .M_UCODE_DBGST_ID = ((0x0*2)+(0x20*2)),
 .M_BCN0_FRM_BYTESZ_ID = ((0x0*2)+(0xc*2)),
 .M_BCN1_FRM_BYTESZ_ID = ((0x0*2)+(0xd*2)),
 .M_DEBUGBLK_PTR_ID = ((0x0*2)+(0x48*2)),
 .M_TXERR_NOWRITE_ID = (((0x109a*2)+(0x0*2))+(0x0*2)),
 .M_TXERR_REASON_ID = INVALID,
 .M_TXERR_REASON2_ID = INVALID,
 .M_TXERR_CTXTST_ID = (((0x109a*2)+(0x0*2))+(0x4*2)),
 .M_TXERR_PCTL0_ID = (((0x109a*2)+(0x0*2))+(0x7*2)),
 .M_TXERR_PCTL1_ID = (((0x109a*2)+(0x0*2))+(0x8*2)),
 .M_TXERR_PCTL2_ID = (((0x109a*2)+(0x0*2))+(0x9*2)),
 .M_TXERR_LSIG0_ID = (((0x109a*2)+(0x0*2))+(0xd*2)),
 .M_TXERR_LSIG1_ID = (((0x109a*2)+(0x0*2))+(0xe*2)),
 .M_TXERR_PLCP0_ID = (((0x109a*2)+(0x0*2))+(0xf*2)),
 .M_TXERR_PLCP1_ID = (((0x109a*2)+(0x0*2))+(0x10*2)),
 .M_TXERR_PLCP2_ID = (((0x109a*2)+(0x0*2))+(0x11*2)),
 .M_TXERR_SIGB0_ID = (((0x109a*2)+(0x0*2))+(0x12*2)),
 .M_TXERR_SIGB1_ID = (((0x109a*2)+(0x0*2))+(0x13*2)),
 .M_TXERR_RXESTATS2_ID = INVALID,
 .M_TXNDPA_CNT_ID = ((0xa52*2)+(0x0*2)),
 .M_TXNDP_CNT_ID = ((0xa52*2)+(0x1*2)),
 .M_TXBFPOLL_CNT_ID = ((0xa52*2)+(0x28*2)),
 .M_TXSF_CNT_ID = ((0xa52*2)+(0x2*2)),
 .M_VHTSU_TXNDPA_CNT_ID = ((0x1ea4*2)+(0x0*2)),
 .M_VHTMU_TXNDPA_CNT_ID = ((0x1ea4*2)+(0x1*2)),
 .M_HESU_TXNDPA_CNT_ID = ((0x1ea4*2)+(0x2*2)),
 .M_HEMU_TXNDPA_CNT_ID = ((0x1ea4*2)+(0x3*2)),
 .M_VHTSU_RXSF_CNT_ID = ((0x1ea4*2)+(0xc*2)),
 .M_VHTMU_RXSF_CNT_ID = ((0x1ea4*2)+(0xd*2)),
 .M_HESU_RXSF_CNT_ID = ((0x1ea4*2)+(0xe*2)),
 .M_HEMU_RXSF_CNT_ID = ((0x1ea4*2)+(0xf*2)),
 .M_VHTSU_EXPRXSF_CNT_ID = ((0x1ea4*2)+(0x6*2)),
 .M_VHTMU_EXPRXSF_CNT_ID = ((0x1ea4*2)+(0x7*2)),
 .M_HESU_EXPRXSF_CNT_ID = ((0x1ea4*2)+(0x8*2)),
 .M_HEMU_EXPRXSF_CNT_ID = ((0x1ea4*2)+(0x9*2)),
 .M_TXCWRTS_CNT_ID = ((0xa52*2)+(0x3*2)),
 .M_TXCWCTS_CNT_ID = ((0xa52*2)+(0x4*2)),
 .M_TXBFM_CNT_ID = ((0xa52*2)+(0x5*2)),
 .M_RXNDPAUCAST_CNT_ID = ((0xa52*2)+(0x6*2)),
 .M_RXNDPAMCAST_CNT_ID = ((0xa52*2)+(0x24*2)),
 .M_RXBFPOLLUCAST_CNT_ID = ((0xa52*2)+(0x25*2)),
 .M_BFERPTRDY_CNT_ID = ((0xa52*2)+(0x7*2)),
 .M_RXSFUCAST_CNT_ID = ((0xa52*2)+(0x8*2)),
 .M_RXCWRTSUCAST_CNT_ID = ((0xa52*2)+(0x9*2)),
 .M_RXCWCTSUCAST_CNT_ID = ((0xa52*2)+(0xa*2)),
 .M_RXDROP20S_CNT_ID = ((0x70*2)+(0x3d*2)),
 .M_RX20S_CNT_ID = ((0xa52*2)+(0xb*2)),
 .M_BTCX_RFACT_CTR_L_ID = ((0xa52*2)+(0x10*2)),
 .M_MACSUSP_CNT_ID = ((0xa52*2)+(0x29*2)),
};
static const d11shm_mbssid_t mbssid_ucode_std_132 = {
 .M_MBSSID_BLK_ID = (0x184*2),
 .M_MBS_BSSID0_ID = ((0x184*2)+(0x0*2)),
 .M_MBS_BSSID1_ID = ((0x184*2)+(0x1*2)),
 .M_MBS_BSSID2_ID = ((0x184*2)+(0x2*2)),
 .M_MBS_NBCN_ID = ((0x184*2)+(0x3*2)),
 .M_MBS_PRQBASE_ID = ((0x184*2)+(0x4*2)),
 .M_MBS_PRETBTT_ID = ((0x184*2)+(0x9*2)),
 .M_MBS_BSSIDNUM_ID = ((0x184*2)+(0xc*2)),
 .M_MBS_PIO_BCBMP_ID = ((0x184*2)+(0xd*2)),
 .M_MBS_PRS_TPLPTR_ID = ((0x184*2)+(0xe*2)),
 .M_MBS_PRSLEN_BLK_ID = ((0x194*2)+(0x0*2)),
 .M_MBS_BCFID_BLK_ID = ((0x194*2)+(0x10*2)),
 .M_MBS_SSIDLEN_BLK_ID = ((0x194*2)+(0x20*2)),
 .M_MBS_SSID_1_ID = ((0x194*2)+(0x0*2)),
 .M_PRQFIFO_RPTR_ID = ((0x0*2)+(0x5e*2)),
 .M_PRQFIFO_WPTR_ID = ((0x0*2)+(0x5f*2)),
 .M_BCN_TPLBLK_BSZ_ID = ((0x0*2)+(0x1d*2)),
};
static const d11shm_physim_t physim_ucode_std_132 = {
 .M_RFLDO_ON_L_ID = ((0x120*2)+(0x5e*2)),
 .M_RFLDO_ON_H_ID = ((0x120*2)+(0x5f*2)),
 .M_PAPDOFF_MCS_ID = INVALID,
 .M_LPF_PASSIVE_RC_OFDM_ID = INVALID,
 .M_LPF_PASSIVE_RC_CCK_ID = INVALID,
 .M_SMPL_COL_BMP_ID = (0x1c7*2),
 .M_SMPL_COL_CTL_ID = (0x1c8*2),
 .M_RT_DIRMAP_A_ID = (0xe0*2),
 .M_RT_DIRMAP_B_ID = (0x100*2),
 .M_CTS_DURATION_ID = ((0x0*2)+(0x5c*2)),
 .M_TSSI_0_ID = INVALID,
 .M_TSSI_1_ID = INVALID,
 .M_IFS_PRICRS_ID = ((0x0*2)+(0x2d*2)),
 .M_TSSI_OFDM_0_ID = INVALID,
 .M_TSSI_OFDM_1_ID = INVALID,
 .M_CURCHANNEL_ID = ((0x0*2)+(0x50*2)),
 .M_PWRIND_BLKS_ID = (0x1d8*2),
 .M_PWRIND_MAP0_ID = ((0x1d8*2)+(0x0*2)),
 .M_PWRIND_MAP1_ID = ((0x1d8*2)+(0x1*2)),
 .M_PWRIND_MAP2_ID = ((0x1d8*2)+(0x2*2)),
 .M_PWRIND_MAP3_ID = ((0x1d8*2)+(0x3*2)),
 .M_PWRIND_MAP4_ID = ((0x1d8*2)+(0x4*2)),
 .M_PWRIND_MAP5_ID = ((0x1d8*2)+(0x5*2)),
 .M_HWACI_STATUS_ID = INVALID,
 .M_HWACI_EN_IND_ID = INVALID,
 .M_FCBS_BLK_ID = (0x1e2*2),
 .M_FCBS_TEMPLATE_LENS_ID = ((0x1e2*2)+(0x0*2)),
 .M_FCBS_BPHY_CTRL_ID = ((0x1e2*2)+(0x4*2)),
 .M_FCBS_TEMPLATE_PTR_ID = ((0x1e2*2)+(0x5*2)),
 .M_AFEOVR_PTR_ID = INVALID,
 .M_RXGAININFO_ANT0_OFFSET_ID = INVALID,
 .M_RXAUXGAININFO_ANT0_OFFSET_ID = INVALID,
 .M_RADAR_REG_ID = ((0x0*2)+(0x33*2)),
};
static const d11shm_ulpphysim_t ulpphysim_ucode_std_132 = {
 .M_RADIOPWRUP_PTR_ID = INVALID,
 .M_RADIOPWRDWN_PTR_ID = INVALID,
 .M_EXT_AVAIL3_ID = INVALID,
 .M_TXPWR_MAX_ID = INVALID,
 .M_TXPWR_CUR_ID = INVALID,
 .M_BCMC_TIMEOUT_ID = ((0x0*2)+(0x13*2)),
 .M_BCMCROLL_TMOUT_ID = ((0x0*2)+(0x27*2)),
 .M_TXPWR_M_ID = INVALID,
 .M_TXPWR_TARGET_ID = INVALID,
 .M_USEQ_PWRUP_PTR_ID = ((0xc0*2)+(0x0*2)),
 .M_USEQ_PWRDN_PTR_ID = ((0xc0*2)+(0x1*2)),
 .M_SLP_RDY_INT_ID = ((0xc0*2)+(0x2*2)),
 .M_TPCNNUM_INTG_LOG2_ID = INVALID,
 .M_TSSI_SENS_LMT1_ID = INVALID,
 .M_TSSI_SENS_LMT2_ID = INVALID,
};
static const d11shm_btcx_t btcx_ucode_std_132 = {
 .M_BTCX_BLK_PTR_ID = ((0x0*2)+(0x49*2)),
 .M_BTCX_PRED_PER_ID = ((0xe9a*2)+(0x4*2)),
 .M_BTCX_LAST_SCO_ID = ((0xe9a*2)+(0xc*2)),
 .M_BTCX_LAST_SCO_H_ID = INVALID,
 .M_BTCX_BLE_SCAN_GRANT_THRESH_ID = ((0xe9a*2)+(0xd*2)),
 .M_BTCX_NEXT_SCO_ID = INVALID,
 .M_BTCX_REQ_START_ID = INVALID,
 .M_BTCX_REQ_START_H_ID = ((0xe9a*2)+(0xf*2)),
 .M_BTCX_LAST_DATA_ID = ((0xe9a*2)+(0x17*2)),
 .M_BTCX_BT_TYPE_ID = ((0xe9a*2)+(0x1b*2)),
 .M_BTCX_ECI0_ID = ((0xe9a*2)+(0x22*2)),
 .M_BTCX_ECI1_ID = ((0xe9a*2)+(0x23*2)),
 .M_BTCX_ECI2_ID = ((0xe9a*2)+(0x24*2)),
 .M_BTCX_ECI3_ID = ((0xe9a*2)+(0x25*2)),
 .M_BTCX_LAST_A2DP_ID = ((0xe9a*2)+(0x26*2)),
 .M_BTCX_PRI_MAP_LO_ID = ((0xe9a*2)+(0x40*2)),
 .M_BTCX_HOLDSCO_LIMIT_OFFSET_ID = (0x44*2),
 .M_BTCX_SCO_GRANT_HOLD_RATIO_OFFSET_ID = (0x46*2),
 .M_BTCX_PRED_COUNT_ID = INVALID,
 .M_BTCX_PROT_RSSI_THRESH_OFFSET_ID = (0x49*2),
 .M_BTCX_PROT_RSSI_THRESH_ID = ((0xe9a*2)+(0x49*2)),
 .M_BTCX_HOST_FLAGS_OFFSET_ID = (0x52*2),
 .M_BTCX_HOST_FLAGS_ID = ((0xe9a*2)+(0x52*2)),
 .M_BTCX_RSSI_ID = ((0xe9a*2)+(0x78*2)),
 .M_BTCX_BT_TXPWR_ID = ((0xe9a*2)+(0x76*2)),
 .M_BTCX_HIGH_THRESH_ID = INVALID,
 .M_BTCX_LOW_THRESH_ID = INVALID,
 .M_BTCX_CONFIG_OFFSET_ID = (0x67*2),
 .M_BTCX_CONFIG_ID = ((0xe9a*2)+(0x67*2)),
 .M_BTCX_NUM_TASKS_OFFSET_ID = INVALID,
 .M_BTCX_NUM_TASKS_ID = INVALID,
 .M_BTCXDBG_BLK_ID = INVALID,
 .M_BTCX_RFSWMSK_BT_OFFSET_ID = (0x6c*2),
 .M_BTCX_RFSWMSK_BT_ID = ((0xe9a*2)+(0x6c*2)),
 .M_BTCX_RFSWMSK_WL_OFFSET_ID = INVALID,
 .M_BTCX_RFSWMSK_WL_ID = INVALID,
 .M_BTCX_AGG_OFF_BM_ID = ((0xe9a*2)+(0x7c*2)),
 .M_BTCX_PKTABORTCTL_VAL_ID = ((0xe9a*2)+(0x77*2)),
 .M_BTCX_BT_TASKS_BM_LOW_ID = ((0xe9a*2)+(0x74*2)),
 .M_BTCX_BT_TASKS_BM_HI_ID = ((0xe9a*2)+(0x75*2)),
 .M_BTCX_ABORT_CNT_ID = INVALID,
 .M_BTCX_LATENCY_CNT_ID = ((0xe9a*2)+(0x36*2)),
 .M_BTCX_HOST_FLAGS2_OFFSET_ID = INVALID,
 .M_BTCX_HOST_FLAGS2_ID = INVALID,
 .M_BTCX_IBSS_TSF_L_ID = INVALID,
 .M_BTCX_IBSS_TSF_ML_ID = INVALID,
 .M_BTCX_IBSS_TSF_SCO_L_ID = INVALID,
 .M_BTCX_SUCC_PM_PROTECT_CNT_ID = INVALID,
 .M_BTCX_SUCC_CTS2A_CNT_ID = INVALID,
 .M_BTCX_WLAN_TX_PREEMPT_CNT_ID = INVALID,
 .M_BTCX_WLAN_RX_PREEMPT_CNT_ID = INVALID,
 .M_BTCX_APTX_AFTER_PM_CNT_ID = INVALID,
 .M_BTCX_PERAUD_CUMU_GRANT_CNT_ID = INVALID,
 .M_BTCX_PERAUD_CUMU_DENY_CNT_ID = INVALID,
 .M_BTCX_A2DP_CUMU_GRANT_CNT_ID = INVALID,
 .M_BTCX_A2DP_CUMU_DENY_CNT_ID = INVALID,
 .M_BTCX_SNIFF_CUMU_GRANT_CNT_ID = INVALID,
 .M_BTCX_SNIFF_CUMU_DENY_CNT_ID = INVALID,
};
static const d11shm_ltecx_t ltecx_ucode_std_132 = {
 .M_LTECX_FLAGS_ID = ((0xe9a*2)+(0x96*2)),
 .M_LTECX_STATE_ID = ((0xe9a*2)+(0x9c*2)),
 .M_LTECX_HOST_FLAGS_ID = ((0xe9a*2)+(0x9d*2)),
 .M_LTECX_TX_LOOKAHEAD_DUR_ID = ((0xe9a*2)+(0xa3*2)),
 .M_LTECX_PROT_ADV_TIME_ID = ((0xe9a*2)+(0xa4*2)),
 .M_LTECX_WCI2_TST_LPBK_NBYTES_TX_ID = ((0xe9a*2)+(0x98*2)),
 .M_LTECX_WCI2_TST_LPBK_NBYTES_ERR_ID = ((0xe9a*2)+(0xb6*2)),
 .M_LTECX_WCI2_TST_LPBK_NBYTES_RX_ID = ((0xe9a*2)+(0xb8*2)),
 .M_LTECX_RX_REAGGR_ID = INVALID,
 .M_LTECX_ACTUALTX_DURATION_ID = ((0xe9a*2)+(0xa7*2)),
 .M_LTECX_CRTI_MSG_ID = ((0xe9a*2)+(0x86*2)),
 .M_LTECX_CRTI_INTERVAL_ID = ((0xe9a*2)+(0x87*2)),
 .M_LTECX_CRTI_REPEATS_ID = ((0xe9a*2)+(0x88*2)),
 .M_LTECX_WCI2_TST_MSG_ID = ((0xe9a*2)+(0xb5*2)),
 .M_LTECX_RXPRI_THRESH_ID = ((0xe9a*2)+(0x8c*2)),
 .M_LTECX_MWSSCAN_BM_LO_ID = ((0xe9a*2)+(0x92*2)),
 .M_LTECX_MWSSCAN_BM_HI_ID = ((0xe9a*2)+(0xb7*2)),
 .M_LTECX_PWRCP_C0_ID = ((0xe9a*2)+(0xbe*2)),
 .M_LTECX_PWRCP_C0_FS_ID = ((0xe9a*2)+(0xbf*2)),
 .M_LTECX_PWRCP_C0_FSANT_ID = ((0xe9a*2)+(0x9a*2)),
 .M_LTECX_PWRCP_C1_ID = INVALID,
 .M_LTECX_PWRCP_C1_FS_ID = INVALID,
 .M_LTECX_FS_OFFSET_ID = INVALID,
 .M_LTECX_TXNOISE_CNT_ID = ((0xe9a*2)+(0xab*2)),
 .M_LTECX_NOISE_DELTA_ID = ((0xe9a*2)+(0x89*2)),
 .M_LTECX_TYPE4_TXINHIBIT_DURATION_ID = INVALID,
 .M_LTECX_TYPE4_NONE_ZERO_CNT_ID = INVALID,
 .M_LTECX_TYPE4_TIMEOUT_CNT_ID = INVALID,
 .M_LTECX_RXPRI_DURATION_ID = INVALID,
 .M_LTECX_RXPRI_CNT_ID = INVALID,
 .M_LTECX_TYP6_DURATION_ID = INVALID,
 .M_LTECX_TYP6_CNT_ID = INVALID,
 .M_LTECX_TS_PROT_FRAME_CNT_ID = INVALID,
 .M_LTECX_TS_GRANT_CNT_ID = INVALID,
 .M_LTECX_TS_GRANT_TIME_DUR_ID = INVALID,
};
static const d11shm_ccastat_t ccastat_ucode_std_132 = {
 .M_CCASTATS_PTR_ID = ((0x0*2)+(0x1b*2)),
 .M_CCA_STATS_BLK_ID = (0xa18*2),
 .M_CCA_TXDUR_L_OFFSET_ID = (0x0*2),
 .M_CCA_TXDUR_H_OFFSET_ID = (0x1*2),
 .M_CCA_INBSS_L_OFFSET_ID = (0x2*2),
 .M_CCA_INBSS_H_OFFSET_ID = (0x3*2),
 .M_CCA_OBSS_L_OFFSET_ID = (0x4*2),
 .M_CCA_OBSS_H_OFFSET_ID = (0x5*2),
 .M_CCA_NOCTG_L_OFFSET_ID = (0x6*2),
 .M_CCA_NOCTG_H_OFFSET_ID = (0x7*2),
 .M_CCA_NOPKT_L_OFFSET_ID = (0x8*2),
 .M_CCA_NOPKT_H_OFFSET_ID = (0x9*2),
 .M_MAC_SLPDUR_L_OFFSET_ID = (0xa*2),
 .M_MAC_SLPDUR_H_OFFSET_ID = (0xb*2),
 .M_CCA_TXOP_L_OFFSET_ID = (0xc*2),
 .M_CCA_TXOP_H_OFFSET_ID = (0xd*2),
 .M_CCA_GDTXDUR_L_OFFSET_ID = (0xe*2),
 .M_CCA_GDTXDUR_H_OFFSET_ID = (0xf*2),
 .M_CCA_BDTXDUR_L_OFFSET_ID = (0x10*2),
 .M_CCA_BDTXDUR_H_OFFSET_ID = (0x11*2),
 .M_CCA_WIFI_L_OFFSET_ID = (0x14*2),
 .M_CCA_WIFI_H_OFFSET_ID = (0x15*2),
 .M_CCA_EDCRSDUR_L_OFFSET_ID = (0x16*2),
 .M_CCA_EDCRSDUR_H_OFFSET_ID = (0x17*2),
 .M_CCA_TXDUR_L_ID = ((0xa18*2)+(0x0*2)),
 .M_CCA_TXDUR_H_ID = ((0xa18*2)+(0x1*2)),
 .M_CCA_INBSS_L_ID = ((0xa18*2)+(0x2*2)),
 .M_CCA_INBSS_H_ID = ((0xa18*2)+(0x3*2)),
 .M_CCA_OBSS_L_ID = ((0xa18*2)+(0x4*2)),
 .M_CCA_OBSS_H_ID = ((0xa18*2)+(0x5*2)),
 .M_CCA_NOCTG_L_ID = ((0xa18*2)+(0x6*2)),
 .M_CCA_NOCTG_H_ID = ((0xa18*2)+(0x7*2)),
 .M_CCA_NOPKT_L_ID = ((0xa18*2)+(0x8*2)),
 .M_CCA_NOPKT_H_ID = ((0xa18*2)+(0x9*2)),
 .M_MAC_SLPDUR_L_ID = ((0xa18*2)+(0xa*2)),
 .M_MAC_SLPDUR_H_ID = ((0xa18*2)+(0xb*2)),
 .M_CCA_TXOP_L_ID = ((0xa18*2)+(0xc*2)),
 .M_CCA_TXOP_H_ID = ((0xa18*2)+(0xd*2)),
 .M_CCA_GDTXDUR_L_ID = ((0xa18*2)+(0xe*2)),
 .M_CCA_GDTXDUR_H_ID = ((0xa18*2)+(0xf*2)),
 .M_CCA_BDTXDUR_L_ID = ((0xa18*2)+(0x10*2)),
 .M_CCA_BDTXDUR_H_ID = ((0xa18*2)+(0x11*2)),
 .M_CCA_RXPRI_LO_ID = ((0xa52*2)+(0x1a*2)),
 .M_CCA_RXPRI_HI_ID = ((0xa52*2)+(0x1b*2)),
 .M_CCA_RXSEC20_LO_ID = ((0xa52*2)+(0x1c*2)),
 .M_CCA_RXSEC20_HI_ID = ((0xa52*2)+(0x1d*2)),
 .M_CCA_RXSEC40_LO_ID = ((0xa52*2)+(0x1e*2)),
 .M_CCA_RXSEC40_HI_ID = ((0xa52*2)+(0x1f*2)),
 .M_CCA_RXSEC80_LO_ID = ((0xa52*2)+(0x20*2)),
 .M_CCA_RXSEC80_HI_ID = ((0xa52*2)+(0x21*2)),
 .M_CCA_SUSP_L_ID = ((0xa18*2)+(0x12*2)),
 .M_CCA_SUSP_H_ID = ((0xa18*2)+(0x13*2)),
 .M_CCA_WIFI_L_ID = ((0xa18*2)+(0x14*2)),
 .M_CCA_WIFI_H_ID = ((0xa18*2)+(0x15*2)),
 .M_CCA_EDCRSDUR_L_ID = ((0xa18*2)+(0x16*2)),
 .M_CCA_EDCRSDUR_H_ID = ((0xa18*2)+(0x17*2)),
 .M_SECRSSI0_ID = ((0xa52*2)+(0xd*2)),
 .M_SECRSSI1_ID = ((0xa52*2)+(0xe*2)),
 .M_SECRSSI2_ID = ((0xa52*2)+(0xf*2)),
 .M_SISO_RXDUR_L_ID = INVALID,
 .M_SISO_RXDUR_H_ID = INVALID,
 .M_SISO_TXOP_L_ID = INVALID,
 .M_SISO_TXOP_H_ID = INVALID,
 .M_MIMO_RXDUR_L_ID = INVALID,
 .M_MIMO_RXDUR_H_ID = INVALID,
 .M_MIMO_TXOP_L_ID = INVALID,
 .M_MIMO_TXOP_H_ID = INVALID,
 .M_MIMO_TXDUR_1X_L_ID = INVALID,
 .M_MIMO_TXDUR_1X_H_ID = INVALID,
 .M_MIMO_TXDUR_2X_L_ID = INVALID,
 .M_MIMO_TXDUR_2X_H_ID = INVALID,
 .M_MIMO_TXDUR_3X_L_ID = INVALID,
 .M_MIMO_TXDUR_3X_H_ID = INVALID,
 .M_SISO_SIFS_L_ID = INVALID,
 .M_SISO_SIFS_H_ID = INVALID,
 .M_MIMO_SIFS_L_ID = INVALID,
 .M_MIMO_SIFS_H_ID = INVALID,
 .M_CCA_TXNODE0_L_OFFSET_ID = INVALID,
 .M_CCA_TXNODE0_H_OFFSET_ID = INVALID,
 .M_CCA_RXNODE0_L_OFFSET_ID = INVALID,
 .M_CCA_RXNODE0_H_OFFSET_ID = INVALID,
 .M_CCA_XXOBSS_L_OFFSET_ID = INVALID,
 .M_CCA_XXOBSS_H_OFFSET_ID = INVALID,
 .M_MESHCCA_STATS_BLK_ID = (0xa30*2),
 .M_MESHCCA_TXNODE0_OFFSET_ID = (0x0*2),
 .M_MESHCCA_RXNODE0_OFFSET_ID = (0x2*2),
 .M_MESHCCA_OBSSNODE_OFFSET_ID = (0x20*2),
 .M_MESHCCA_TXNODE0_ID = ((0xa30*2)+(0x0*2)),
 .M_MESHCCA_RXNODE0_ID = ((0xa30*2)+(0x2*2)),
 .M_MESHCCA_OBSSNODE_ID = ((0xa30*2)+(0x20*2)),
 .M_HUGENAV_RTS_CNT_ID = ((0xa52*2)+(0x22*2)),
 .M_HUGENAV_CTS_CNT_ID = ((0xa52*2)+(0x23*2)),
};
static const d11shm_ampdu_t ampdu_ucode_std_132 = {
 .M_TXFS_PTR_ID = INVALID,
 .M_AMP_STATS_PTR_ID = ((0x0*2)+(0x3d*2)),
 .M_MIMO_MAXSYM_ID = ((0x0*2)+(0x5d*2)),
 .M_WATCHDOG_8TU_ID = ((0x0*2)+(0x1e*2)),
};
static const d11shm_btamp_t btamp_ucode_std_132 = {
 .M_BTAMP_GAIN_DELTA_ID = INVALID,
};
static const d11shm_rev_ge_40_t rev_ge_40_ucode_std_132 = {
 .M_BCN_TXPCTL0_ID = ((0x0*2)+(0x66*2)),
 .M_BCN_TXPCTL1_ID = ((0x0*2)+(0x67*2)),
 .M_BCN_TXPCTL2_ID = ((0x0*2)+(0x68*2)),
 .M_RSP_TXPCTL0_ID = ((0x0*2)+(0x4c*2)),
 .M_RSP_TXPCTL1_ID = ((0x0*2)+(0x4d*2)),
 .M_UPRS_INTVL_L_ID = ((0x206*2)+(0x2*2)),
 .M_UPRS_INTVL_H_ID = ((0x206*2)+(0x3*2)),
 .M_UPRS_FD_TSF_LOC_ID = ((0x206*2)+(0x4*2)),
 .M_UPRS_FD_TXTSF_OFFSET_ID = ((0x206*2)+(0x5*2)),
 .M_TXDUTY_RATIOX16_CCK_ID = ((0x0*2)+(0x52*2)),
 .M_TXDUTY_RATIOX16_OFDM_ID = ((0x0*2)+(0x5a*2)),
};
static const d11shm_rev_lt_40_t rev_lt_40_ucode_std_132 = {
 .M_BCN_PCTLWD_ID = INVALID,
 .M_BCN_PCTL1WD_ID = INVALID,
 .M_SWDIV_SWCTRL_REG_ID = INVALID,
 .M_SWDIV_PREF_ANT_ID = INVALID,
 .M_SWDIV_TX_PREF_ANT_ID = INVALID,
 .M_LCNXN_SWCTRL_MASK_ID = INVALID,
 .M_4324_RXTX_WAR_PTR_ID = INVALID,
 .M_TX_MODE_0xb0_OFFSET_ID = INVALID,
 .M_TX_MODE_0x14d_OFFSET_ID = INVALID,
 .M_TX_MODE_0xb1_OFFSET_ID = INVALID,
 .M_TX_MODE_0x14e_OFFSET_ID = INVALID,
 .M_TX_MODE_0xb4_OFFSET_ID = INVALID,
 .M_TX_MODE_0x151_OFFSET_ID = INVALID,
 .M_RX_MODE_0xb0_OFFSET_ID = INVALID,
 .M_RX_MODE_0x14d_OFFSET_ID = INVALID,
 .M_RX_MODE_0xb1_OFFSET_ID = INVALID,
 .M_RX_MODE_0x14e_OFFSET_ID = INVALID,
 .M_RX_MODE_0xb4_OFFSET_ID = INVALID,
 .M_RX_MODE_0x151_OFFSET_ID = INVALID,
 .M_CHIP_CHECK_OFFSET_ID = INVALID,
 .M_CTXPRS_BLK_ID = INVALID,
 .M_RADIO_PWR_ID = INVALID,
 .M_IFSCTL1_ID = INVALID,
 .M_TX_IDLE_BUSY_RATIO_X_16_CCK_ID = INVALID,
 .M_TX_IDLE_BUSY_RATIO_X_16_OFDM_ID = INVALID,
 .M_RSP_PCTLWD_ID = INVALID,
 .M_BCN_POWER_ADJUST_ID = INVALID,
 .M_PRS_POWER_ADJUST_ID = INVALID,
};
static const d11shm_bcntrim_t bcntrim_ucode_std_132 = {
 .M_BCNTRIM_BLK_ID = (0x110c*2),
 .M_BCNTRIM_PER_OFFSET_ID = (0xb*2),
 .M_BCNTRIM_TIMEND_OFFSET_ID = (0xc*2),
 .M_BCNTRIM_TSFLMT_OFFSET_ID = (0xd*2),
 .M_BCNTRIM_CNT_OFFSET_ID = (0xc*2),
 .M_BCNTRIM_RSSI_OFFSET_ID = INVALID,
 .M_BCNTRIM_CHAN_OFFSET_ID = INVALID,
 .M_BCNTRIM_SNR_OFFSET_ID = INVALID,
 .M_BCNTRIM_CUR_OFFSET_ID = (0x0*2),
 .M_BCNTRIM_PREVLEN_OFFSET_ID = (0x1*2),
 .M_BCNTRIM_TIMLEN_OFFSET_ID = (0x2*2),
 .M_BCNTRIM_RXMBSS_OFFSET_ID = INVALID,
 .M_BCNTRIM_TIMNOTFOUND_OFFSET_ID = INVALID,
 .M_BCNTRIM_CANTRIM_OFFSET_ID = INVALID,
 .M_BCNTRIM_LENCHG_OFFSET_ID = INVALID,
 .M_BCNTRIM_TSFDRF_OFFSET_ID = INVALID,
 .M_BCNTRIM_TIMBITSET_OFFSET_ID = INVALID,
 .M_BCNTRIM_WAKE_OFFSET_ID = INVALID,
 .M_BCNTRIM_SSID_OFFSET_ID = INVALID,
 .M_BCNTRIM_DTIM_OFFSET_ID = INVALID,
 .M_BCNTRIM_SSIDBLK_OFFSET_ID = INVALID,
};
static const d11shm_rev_sslp_lt_40_t rev_sslp_lt_40_ucode_std_132 = {
 .M_PHY_NOISE_ID = ((0x0*2)+(0x37*2)),
 .M_RSSI_LOCK_OFFSET_ID = INVALID,
 .M_RSSI_LOGNSAMPS_OFFSET_ID = INVALID,
 .M_RSSI_NSAMPS_OFFSET_ID = INVALID,
 .M_RSSI_IQEST_EN_OFFSET_ID = INVALID,
 .M_RSSI_BOARDATTEN_DBG_OFFSET_ID = INVALID,
 .M_RSSI_IQPWR_DBG_OFFSET_ID = INVALID,
 .M_RSSI_IQPWR_DB_DBG_OFFSET_ID = INVALID,
 .M_NOISE_IQPWR_ID = INVALID,
 .M_NOISE_IQPWR_OFFSET_ID = INVALID,
 .M_NOISE_IQPWR_DB_OFFSET_ID = INVALID,
 .M_NOISE_LOGNSAMPS_ID = INVALID,
 .M_NOISE_LOGNSAMPS_OFFSET_ID = INVALID,
 .M_NOISE_NSAMPS_ID = INVALID,
 .M_NOISE_NSAMPS_OFFSET_ID = INVALID,
 .M_NOISE_IQEST_EN_OFFSET_ID = INVALID,
 .M_NOISE_IQEST_PENDING_ID = INVALID,
 .M_NOISE_IQEST_PENDING_OFFSET_ID = INVALID,
 .M_RSSI_IQEST_PENDING_OFFSET_ID = INVALID,
 .M_NOISE_LTE_ON_ID = INVALID,
 .M_NOISE_LTE_IQPWR_DB_OFFSET_ID = INVALID,
 .M_SSLPN_RSSI_0_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_0_logchPowAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_0_errAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_RSSI_1_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_1_logchPowAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_1_errAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_RSSI_2_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_2_logchPowAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_2_errAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_RSSI_3_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_3_logchPowAccOut_OFFSET_ID = INVALID,
 .M_SSLPN_SNR_3_errAccOut_OFFSET_ID = INVALID,
 .M_RSSI_QDB_0_OFFSET_ID = INVALID,
 .M_RSSI_QDB_1_OFFSET_ID = INVALID,
 .M_RSSI_QDB_2_OFFSET_ID = INVALID,
 .M_RSSI_QDB_3_OFFSET_ID = INVALID,
};
static const d11shm_wowl_t wowl_ucode_std_132 = {
 .M_WOWL_TEST_CYCLE_ID = INVALID,
 .M_WOWL_TMR_L_ID = INVALID,
 .M_WOWL_TMR_ML_ID = INVALID,
 .M_KEYRC_LAST_ID = INVALID,
 .M_NETPAT_BLK_PTR_ID = INVALID,
 .M_WOWL_GPIOSEL_ID = INVALID,
 .M_NETPAT_NUM_ID = INVALID,
 .M_AESTABLES_PTR_ID = INVALID,
 .M_AID_NBIT_ID = ((0x0*2)+(0x62*2)),
 .M_HOST_WOWLBM_ID = INVALID,
 .M_GROUPKEY_UPBM_ID = INVALID,
 .M_WOWL_OFFLOADCFG_PTR_ID = INVALID,
 .M_CTX_GTKMSG2_ID = INVALID,
 .M_TXPHYERR_CNT_ID = ((0x70*2)+(0xf*2)),
 .M_SECSUITE_ID = INVALID,
 .M_TSCPN_BLK_ID = INVALID,
 .M_WAKEEVENT_IND_ID = INVALID,
 .M_EAPOLMICKEY_BLK_ID = INVALID,
 .M_RXFRM_SRA0_ID = INVALID,
 .M_RXFRM_SRA1_ID = INVALID,
 .M_RXFRM_SRA2_ID = INVALID,
 .M_RXFRM_RA0_ID = INVALID,
 .M_RXFRM_RA1_ID = INVALID,
 .M_RXFRM_RA2_ID = INVALID,
 .M_TXPSPFRM_CNT_ID = INVALID,
 .M_WOWL_NOBCN_ID = INVALID,
};
static const d11shm_bcmulp_t bcmulp_ucode_std_132 = {
 .M_DRVR_UCODE_IF_PTR_ID = INVALID,
 .M_ULP_FEATURES_OFFSET_ID = INVALID,
 .M_DRIVER_BLOCK_OFFSET_ID = INVALID,
 .M_CRX_BLK_ID = INVALID,
 .M_RXFRM_BASE_ADDR_ID = (((0x68c*2)+(0x14*2))+(0x1*2)),
 .M_SAVERESTORE_4335_BLK_ID = (0x1114*2),
 .M_ILP_PER_H_OFFSET_ID = (0x0*2),
 .M_ILP_PER_L_OFFSET_ID = (0x1*2),
 .M_DRIVER_BLOCK_ID = INVALID,
 .M_FCBS_DS1_MAC_INIT_BLOCK_OFFSET_ID = INVALID,
 .M_FCBS_DS1_PHY_RADIO_BLOCK_OFFSET_ID = INVALID,
 .M_FCBS_DS1_RADIO_PD_BLOCK_OFFSET_ID = INVALID,
 .M_FCBS_DS1_EXIT_BLOCK_OFFSET_ID = INVALID,
 .M_FCBS_DS0_RADIO_PU_BLOCK_OFFSET_ID = INVALID,
 .M_FCBS_DS0_RADIO_PD_BLOCK_OFFSET_ID = INVALID,
 .M_ULP_WAKE_IND_ID = INVALID,
 .M_ILP_PER_H_ID = ((0x1114*2)+(0x0*2)),
 .M_ILP_PER_L_ID = ((0x1114*2)+(0x1*2)),
 .M_DS1_CTRL_SDIO_PIN_ID = INVALID,
 .M_DS1_CTRL_SDIO_ID = INVALID,
 .M_RXBEACONMBSS_CNT_ID = ((0x70*2)+(0x27*2)),
 .M_FRRUN_LBCN_CNT_ID = INVALID,
 .M_FCBS_DS0_RADIO_PU_BLOCK_ID = INVALID,
 .M_FCBS_DS0_RADIO_PD_BLOCK_ID = INVALID,
};
static const d11shm_tsync_t tsync_ucode_std_132 = {
 .M_TS_SYNC_GPIO_ID = INVALID,
 .M_TS_SYNC_TSF_L_ID = INVALID,
 .M_TS_SYNC_TSF_ML_ID = INVALID,
 .M_TS_SYNC_AVB_L_ID = INVALID,
 .M_TS_SYNC_AVB_H_ID = INVALID,
 .M_TS_SYNC_PMU_L_ID = INVALID,
 .M_TS_SYNC_PMU_H_ID = INVALID,
 .M_TS_SYNC_TXTSF_ML_ID = INVALID,
 .M_TS_SYNC_GPIOMINDLY_ID = INVALID,
 .M_TS_SYNC_GPIOREALDLY_ID = INVALID,
};
static const d11shm_ops_t ops_ucode_std_132 = {
 .M_OPS_MODE_ID = INVALID,
 .M_OPS_RSSI_THRSH_ID = INVALID,
 .M_OPS_MAX_LMT_ID = INVALID,
 .M_OPS_HIST_ID = INVALID,
 .M_OPS_LIGHT_L_ID = INVALID,
 .M_OPS_LIGHT_H_ID = INVALID,
 .M_OPS_FULL_L_ID = INVALID,
 .M_OPS_FULL_H_ID = INVALID,
 .M_OPS_NAV_CNT_ID = INVALID,
 .M_OPS_PLCP_CNT_ID = INVALID,
 .M_OPS_RSSI_CNT_ID = INVALID,
 .M_OPS_MISS_CNT_ID = INVALID,
 .M_OPS_MAXLMT_CNT_ID = INVALID,
 .M_OPS_MYBSS_CNT_ID = INVALID,
 .M_OPS_OBSS_CNT_ID = INVALID,
 .M_OPS_WAKE_CNT_ID = INVALID,
 .M_OPS_BCN_CNT_ID = INVALID,
};
static const d11shm_rev_ge_64_t rev_ge_64_ucode_std_132 = {
 .M_UCODE_MACSTAT_ID = INVALID,
 .M_UCODE_MACSTAT1_PTR_ID = INVALID,
 .M_SYNTHPU_DLY_ID = INVALID,
 .MX_PSM_SOFT_REGS_ID = (0x0*2),
 .MX_BOM_REV_MAJOR_ID = ((0x0*2)+(0x0*2)),
 .MX_BOM_REV_MINOR_ID = ((0x0*2)+(0x1*2)),
 .MX_UCODE_FEATURES_ID = ((0x0*2)+(0x5*2)),
 .MX_UCODE_DATE_ID = INVALID,
 .MX_UCODE_TIME_ID = INVALID,
 .MX_UCODE_DBGST_ID = ((0x0*2)+(0x6*2)),
 .MX_WATCHDOG_8TU_ID = ((0x0*2)+(0x7*2)),
 .MX_MACHW_VER_ID = ((0x0*2)+(0x8*2)),
 .MX_PHYVER_ID = ((0x0*2)+(0x9*2)),
 .MX_PHYTYPE_ID = ((0x0*2)+(0xa*2)),
 .MX_HOST_FLAGS0_ID = ((0x0*2)+(0xb*2)),
 .MX_HOST_FLAGS1_ID = ((0x0*2)+(0xc*2)),
 .MX_HOST_FLAGS2_ID = ((0x0*2)+(0xd*2)),
 .MX_BFI_BLK_ID = (0x34c*2),
 .MX_NDPPWR_TBL_ID = INVALID,
 .MX_VMU_NDPPWR_TBL_ID = (0xc20*2),
 .MX_HMU_NDPPWR_TBL_ID = (0xc26*2),
 .MX_MUSND_PER_ID = (((0x0*2)+(0x30*2))+(0x0*2)),
 .MX_UTRACE_SPTR_ID = ((0x0*2)+(0x24*2)),
 .MX_UTRACE_EPTR_ID = ((0x0*2)+(0x25*2)),
 .M_AGGMPDU_HISTO_ID = ((0xf64*2)+(0x0*2)),
 .M_AGGSTOP_HISTO_ID = ((0xf64*2)+(0x100*2)),
 .M_MBURST_HISTO_ID = ((0xf64*2)+(0x108*2)),
 .M_TXBCN_DUR_ID = ((0x0*2)+(0x16*2)),
 .M_PHYPREEMPT_VAL_ID = ((0xc0*2)+(0x4*2)),
};
static const d11shm_rev_ge_128_t rev_ge_128_ucode_std_132 = {
 .M_COREMASK_HETB_ID = ((0x1ca*2)+(0x4*2)),
 .M_RXTRIG_CMNINFO_ID = ((0x1168*2)+(0x0*2)),
 .M_RXTRIG_USRINFO_ID = ((0x1168*2)+(0x4*2)),
 .M_TXERR_PHYSTS_ID = (((0x109a*2)+(0x0*2))+(0x1*2)),
 .M_TXERR_REASON0_ID = (((0x109a*2)+(0x0*2))+(0x2*2)),
 .M_TXERR_REASON1_ID = (((0x109a*2)+(0x0*2))+(0x3*2)),
 .M_TXERR_TXDUR_ID = (((0x109a*2)+(0x0*2))+(0x5*2)),
 .M_TXERR_PCTLEN_ID = (((0x109a*2)+(0x0*2))+(0x6*2)),
 .M_TXERR_PCTL4_ID = (((0x109a*2)+(0x0*2))+(0xa*2)),
 .M_TXERR_PCTL9_ID = (((0x109a*2)+(0x0*2))+(0xb*2)),
 .M_TXERR_PCTL10_ID = (((0x109a*2)+(0x0*2))+(0xc*2)),
 .M_TXERR_CCLEN_ID = (((0x109a*2)+(0x0*2))+(0x15*2)),
 .M_TXERR_TXBYTES_L_ID = (((0x109a*2)+(0x0*2))+(0x16*2)),
 .M_TXERR_TXBYTES_H_ID = (((0x109a*2)+(0x0*2))+(0x17*2)),
 .M_TXERR_UNFLSTS_ID = (((0x109a*2)+(0x0*2))+(0x18*2)),
 .M_TXERR_USR_ID = (((0x109a*2)+(0x0*2))+(0x19*2)),
 .M_RXTRIG_MYAID_CNT_ID = ((0xa52*2)+(0x34*2)),
 .M_RXTRIG_RAND_CNT_ID = ((0xa52*2)+(0x35*2)),
 .M_RXSWRST_CNT_ID = ((0xa52*2)+(0x2a*2)),
 .M_RXSFCQI_CNT_ID = ((0xa52*2)+(0x3e*2)),
 .M_NDPAUSR_CNT_ID = ((0x13d6*2)+(0x13*2)),
 .M_BFD_DONE_CNT_ID = ((0xa52*2)+(0x26*2)),
 .M_BFD_FAIL_CNT_ID = ((0xa52*2)+(0x27*2)),
 .M_RXSFERR_CNT_ID = ((0x13d6*2)+(0x17*2)),
 .M_RXPFFLUSH_CNT_ID = ((0xa52*2)+(0x2b*2)),
 .M_RXFLUCMT_CNT_ID = ((0xa52*2)+(0x2d*2)),
 .M_RXFLUOV_CNT_ID = ((0xa52*2)+(0x2e*2)),
 .MX_HEMSCH_BLKS_ID = (0xaa0*2),
 .MX_HEMSCH0_BLK_ID = ((0xaa0*2)+(0x0*2)),
 .MX_HEMSCH0_SIGA_ID = (((0xaa0*2)+(0x0*2))+(0x1*2)),
 .MX_HEMSCH0_PCTL0_ID = (((0xaa0*2)+(0x0*2))+(0x4*2)),
 .MX_HEMSCH0_N_ID = (((0xaa0*2)+(0x0*2))+(0x9*2)),
 .MX_HEMSCH0_USR_ID = (((0xaa0*2)+(0x0*2))+(0xa*2)),
 .MX_HEMSCH0_URDY0_ID = INVALID,
 .M_TXTRIG_FLAG_ID = ((0x118c*2)+(0x0*2)),
 .M_TXTRIG_NUM_ID = ((0x118c*2)+(0x1*2)),
 .M_TXTRIG_LEN_ID = ((0x118c*2)+(0x4*2)),
 .M_TXTRIG_RATE_ID = ((0x118c*2)+(0x5*2)),
 .M_MUAGG_HISTO_ID = ((0xf64*2)+(0x110*2)),
 .M_HEMMUAGG_HISTO_ID = ((0xf64*2)+(0x115*2)),
 .M_HEOMUAGG_HISTO_ID = ((0xf64*2)+(0x11a*2)),
 .M_TXAMPDUSU_CNT_ID = ((0xf64*2)+(0x12b*2)),
 .M_TXTRIG_MINTIME_ID = ((0x118c*2)+(0x6*2)),
 .M_TXTRIG_FRAME_ID = ((0x118c*2)+(0x1c*2)),
 .M_TXTRIG_CMNINFO_ID = ((0x118c*2)+(0x24*2)),
 .M_ULRXCTL_BLK_ID = INVALID,
 .M_RTS_MINLEN_L_ID = ((0x0*2)+(0x2a*2)),
 .M_RTS_MINLEN_H_ID = ((0x0*2)+(0x2b*2)),
 .M_AGG0_CNT_ID = ((0xa52*2)+(0x30*2)),
 .M_TRIGREFILL_CNT_ID = ((0x13d6*2)+(0x0*2)),
 .M_TXTRIG_CNT_ID = ((0x13d6*2)+(0x1*2)),
 .M_RXHETBBA_CNT_ID = ((0x13d6*2)+(0x2*2)),
 .M_TXBAMTID_CNT_ID = ((0x13d6*2)+(0x3*2)),
 .M_TXBAMSTA_CNT_ID = ((0x13d6*2)+(0x4*2)),
 .M_TXMBA_CNT_ID = INVALID,
 .M_RXMYDTINRSP_ID = ((0x13d6*2)+(0x6*2)),
 .M_RXEXIT_CNT_ID = ((0x13d6*2)+(0x7*2)),
 .M_PHYRXSFULL_CNT_ID = ((0x13d6*2)+(0x9*2)),
 .M_BFECAP_HE_ID = ((0x21a*2)+(0x6*2)),
 .M_BFECAP_VHT_ID = ((0x21a*2)+(0x5*2)),
 .M_BFECAP_HT_ID = ((0x21a*2)+(0x4*2)),
 .M_BSS_BLK_ID = ((0x21a*2)+(0x7*2)),
 .MX_MUBFI_BLK_ID = INVALID,
 .M_TXTRIG_SRXCTL_ID = ((0x118c*2)+(0x8*2)),
 .M_TXTRIG_SRXCTLUSR_ID = ((0x118c*2)+(0xc*2)),
 .M_ULTX_STS_ID = ((0x1416*2)+(0x4*2)),
 .M_ULTX_ACMASK_ID = ((0x1416*2)+(0x5*2)),
 .M_BCN_TXPCTL6_ID = ((0xc0*2)+(0x8*2)),
 .MX_OQEXPECTN_BLK_ID = ((0xc2c*2)+(0x0*2)),
 .MX_OQMAXN_BLK_ID = ((0xc2c*2)+(0x4*2)),
 .MX_OMSCH_TMOUT_ID = ((0x0*2)+(0x27*2)),
 .MX_SNDREQ_BLK_ID = (0x754*2),
 .M_BFI_GENCFG_ID = ((0x21a*2)+(0x0*2)),
 .MX_TRIG_TXCFG_ID = (0xba0*2),
 .MX_TRIG_TXLMT_ID = ((0x88*2)+(0x4*2)),
 .M_PHYREG_TDSFO_VAL_ID = ((0xc0*2)+(0x9*2)),
 .M_PHYREG_HWOBSS_VAL_ID = ((0xc0*2)+(0x1d*2)),
 .M_PHYREG_TX_SHAPER_COMMON12_VAL_ID = ((0xc0*2)+(0x1e*2)),
 .M_BSS_BSRT_BLK_ID = (0x1d00*2),
 .M_STXVM_BLK_ID = (0x1c26*2),
 .MX_TXVBMP_BLK_ID = (0x1c8*2),
 .MX_MRQ_UPDPEND_ID = ((0x40*2)+(0x3b*2)),
 .MX_TXVM_BLK_ID = (0x148*2),
 .MX_CQIBMP_BLK_ID = (0x264*2),
 .MX_CQIM_BLK_ID = (0x1e4*2),
 .M_TXBRPTRIG_CNT_ID = ((0xa52*2)+(0x31*2)),
 .MX_MACREQ_BLK_ID = (0xbc*2),
 .M_TXVMSTATS_BLK_ID = (0x1c40*2),
 .M_CQIMSTATS_BLK_ID = (0x1cc0*2),
 .M_TXVFULL_CNT_ID = ((0x1c26*2)+(0x12*2)),
 .MX_SNDAGE_THRSH_ID = (((0x0*2)+(0x30*2))+(0x1*2)),
 .M_HETB_CSTHRSH_LO_ID = (0x141f*2),
 .M_HETB_CSTHRSH_HI_ID = (0x1420*2),
 .MX_CURCHANNEL_ID = ((0x0*2)+(0x2d*2)),
 .MX_OQMINN_BLK_ID = ((0xc2c*2)+(0x8*2)),
 .M_D11SR_BLK_ID = (0x1ea*2),
 .M_D11SR_NSRG_PDMIN_ID = ((0x1ea*2)+(0x0*2)),
 .M_D11SR_NSRG_PDMAX_ID = ((0x1ea*2)+(0x1*2)),
 .M_D11SR_SRG_PDMIN_ID = ((0x1ea*2)+(0x2*2)),
 .M_D11SR_SRG_PDMAX_ID = ((0x1ea*2)+(0x3*2)),
 .M_D11SR_TXPWRREF_ID = ((0x1ea*2)+(0x4*2)),
 .M_D11SR_NSRG_TXPWRREF0_ID = ((0x1ea*2)+(0x5*2)),
 .M_D11SR_SRG_TXPWRREF0_ID = ((0x1ea*2)+(0x6*2)),
 .M_D11SR_OPTIONS_ID = ((0x1ea*2)+(0x9*2)),
 .M_D11SROPP_CNT_ID = ((0x1ea*2)+(0x7*2)),
 .M_D11SRTX_CNT_ID = ((0x1ea*2)+(0x8*2)),
 .M_TWTCMD_ID = ((0x1d20*2)+(0x0*2)),
 .M_TWTINT_DATA_ID = ((0x1d20*2)+(0x1*2)),
 .M_PRETWT_US_ID = ((0x1d20*2)+(0x2*2)),
 .M_TWT_PRESTRT_ID = ((0x1d20*2)+(0x5*2)),
 .M_TWT_PRESTOP_ID = ((0x1d20*2)+(0x6*2)),
 .MX_TWT_PRESTRT_ID = (0x753*2),
 .MX_ULOMAXN_BLK_ID = ((0xc38*2)+(0x0*2)),
 .M_TXTRIGWT0_VAL_ID = ((0x114c*2)+(0x0*2)),
 .M_TXTRIGWT1_VAL_ID = ((0x114c*2)+(0x1*2)),
 .MX_ULC_NUM_ID = ((0x0*2)+(0x10*2)),
 .MX_M2VMSG_CNT_ID = ((0x40*2)+(0x1*2)),
 .MX_V2MMSG_CNT_ID = ((0x40*2)+(0x2*2)),
 .MX_M2VGRP_CNT_ID = ((0x40*2)+(0x6*2)),
 .MX_V2MGRP_CNT_ID = ((0x40*2)+(0x7*2)),
 .MX_V2MGRPINV_CNT_ID = ((0x40*2)+(0x8*2)),
 .MX_M2VSND_CNT_ID = ((0x40*2)+(0x9*2)),
 .MX_V2MSND_CNT_ID = ((0x40*2)+(0xa*2)),
 .MX_V2MSNDINV_CNT_ID = ((0x40*2)+(0xb*2)),
 .MX_M2VCQI_CNT_ID = ((0x40*2)+(0x38*2)),
 .MX_V2MCQI_CNT_ID = ((0x40*2)+(0x39*2)),
 .MX_FFQADD_CNT_ID = ((0x40*2)+(0x10*2)),
 .MX_FFQDEL_CNT_ID = ((0x40*2)+(0x11*2)),
 .MX_MFQADD_CNT_ID = ((0x40*2)+(0x12*2)),
 .MX_MFQDEL_CNT_ID = ((0x40*2)+(0x15*2)),
 .MX_MFOQADD_CNT_ID = ((0x40*2)+(0x13*2)),
 .MX_MFOQDEL_CNT_ID = ((0x40*2)+(0x16*2)),
 .MX_OFQADD_CNT_ID = ((0x40*2)+(0x14*2)),
 .MX_OFQDEL_CNT_ID = ((0x40*2)+(0x17*2)),
 .MX_RUCFG_CNT_ID = ((0x40*2)+(0x37*2)),
 .MX_M2VRU_CNT_ID = ((0x40*2)+(0x30*2)),
 .MX_V2MRU_CNT_ID = ((0x40*2)+(0x31*2)),
 .MX_OFQAGG0_CNT_ID = ((0x40*2)+(0x36*2)),
 .MX_ULO_QNULLTHRSH_ID = ((0xc38*2)+(0x4*2)),
 .MX_MACREQ_CNT_ID = ((0x40*2)+(0x33*2)),
 .MX_SNDFL_CNT_ID = ((0x40*2)+(0x3e*2)),
 .MX_M2SQ0_CNT_ID = ((0x40*2)+(0x18*2)),
 .MX_M2SQ1_CNT_ID = ((0x40*2)+(0x19*2)),
 .MX_M2SQ2_CNT_ID = ((0x40*2)+(0x1a*2)),
 .MX_M2SQ3_CNT_ID = ((0x40*2)+(0x1b*2)),
 .MX_M2SQ4_CNT_ID = ((0x40*2)+(0x1c*2)),
 .MX_HEMUCAPINV_CNT_ID = ((0x40*2)+(0x1d*2)),
 .MX_M2SQTXVEVT_CNT_ID = ((0x40*2)+(0x1e*2)),
 .MX_MMUREGRP_CNT_ID = ((0x40*2)+(0x1f*2)),
 .MX_MMUEMGCGRP_CNT_ID = ((0x40*2)+(0xc*2)),
 .MX_M2VGRPWAIT_CNT_ID = ((0x40*2)+(0x2e*2)),
 .MX_OM2SQ0_CNT_ID = ((0x40*2)+(0x22*2)),
 .MX_OM2SQ1_CNT_ID = ((0x40*2)+(0x23*2)),
 .MX_OM2SQ2_CNT_ID = ((0x40*2)+(0x24*2)),
 .MX_OM2SQ3_CNT_ID = ((0x40*2)+(0x25*2)),
 .MX_OM2SQ4_CNT_ID = ((0x40*2)+(0x26*2)),
 .MX_OM2SQ5_CNT_ID = ((0x40*2)+(0x27*2)),
 .MX_OM2SQ6_CNT_ID = ((0x40*2)+(0x3f*2)),
 .MX_TAFWMISMATCH_CNT_ID = ((0x40*2)+(0x3d*2)),
 .MX_OMREDIST_CNT_ID = ((0x40*2)+(0x3c*2)),
 .MX_OMUTXDLMEMTCH_CNT_ID = ((0x40*2)+(0x28*2)),
 .M_CSI_STATUS_ID = ((0x0*2)+(0x63*2)),
 .M_CSI_BLKS_ID = (0xe88*2),
 .M_TXMURTS_CNT_ID = ((0xa52*2)+(0x3d*2)),
 .M_TXTRIGAMPDU_CNT_ID = ((0x13d6*2)+(0x16*2)),
 .M_TXMUBAR_CNT_ID = ((0x13d6*2)+(0x15*2)),
 .M_BSS_BCNPRS_PWR_BLK_ID = (0x1f6*2),
 .M_PPR_BLK_ID = (0x1d30*2),
 .M_MAX_TXPWR_ID = ((0x0*2)+(0x58*2)),
 .M_BRDLMT_BLK_ID = (0x1d38*2),
 .M_RULMT_BLK_ID = (0x1d56*2),
 .MX_SCHED_HIST_ID = ((0xd90*2)+(0x4*2)),
 .MX_SCHED_MAX_ID = ((0xd90*2)+(0xc*2)),
 .M_PSMWDS_PC_ID = ((0xc0*2)+(0x10*2)),
 .MX_PSMWDS_PC_ID = (0x9ca*2),
 .M_TXQNL_CNT_ID = ((0x13d6*2)+(0x1b*2)),
 .M_HETBFP_CNT_ID = ((0x13d6*2)+(0x1c*2)),
 .M_TXHETBBA_CNT_ID = ((0x13d6*2)+(0x1d*2)),
 .MX_ULOFIFO_BASE_ID = ((0xaca*2)+(0x0*2)),
 .MX_ULOFIFO_BMP_ID = ((0xaca*2)+(0x1*2)),
 .M_NAV_MAX_VAL_ID = ((0xc0*2)+(0xf*2)),
 .M_RPTOVRD_CNT_ID = (0x1e94*2),
 .MX_GRP_HIST_BLK_ID = (0x1f68*2),
 .MX_FFQ_GAP_BLK_ID = (0x2028*2),
 .M_ULTX_HOLDTM_L_ID = ((0x1416*2)+(0x2*2)),
 .M_ULTX_HOLDTM_H_ID = ((0x1416*2)+(0x3*2)),
 .M_CUR_BSSCOLOR_ID = ((0x20c*2)+(0x0*2)),
 .M_MY_BSSCOLOR_ID = ((0x20c*2)+(0x1*2)),
 .M_COCLS_CNT_ID = ((0x20c*2)+(0x2*2)),
 .M_SKIP_FFQCONS_CNT_ID = ((0x13d6*2)+(0xe*2)),
 .MX_AGGX_SUTHRSH_ID = ((0x2078*2)+(0x1*2)),
 .MX_AGGX_MUTHRSH_ID = ((0x2078*2)+(0x2*2)),
 .M_RSSICOR_BLK_ID = (0x1d2e*2),
};
static const d11shm_eap_t eap_ucode_std_132 = {
 .M_DTIM_TXENQD_BMBP_ID = INVALID,
 .M_DTIMCNT_MBSS_BMBP_ID = INVALID,
 .M_ODAP_ACK_TMOUT_ID = INVALID,
 .M_ODAP_ACK_TMOUT_TEST_ID = INVALID,
 .M_MBS_DAGGOFF_BMP_ID = INVALID,
 .M_CTRL_CHANNEL_ID = INVALID,
 .M_ENT_HOST_FLAGS1_ID = INVALID,
 .M_EAP_BLK_0_ID = INVALID,
 .M_EAP_BLK_1_ID = INVALID,
 .M_UCODE_CAP_L_ID = INVALID,
 .M_UCODE_CAP_H_ID = INVALID,
 .M_UCODE_FEATURE_EN_L_ID = INVALID,
 .M_UCODE_FEATURE_EN_H_ID = INVALID,
 .M_PRS_RETRY_THR_ID = ((0x0*2)+(0x14*2)),
 .M_MBSS_CCK_BITMAP_ID = INVALID,
 .M_GEN_DBG0_ID = INVALID,
 .M_EAP_BLK_2_ID = INVALID,
 .M_SAS_DEBUG_BLK_ID = INVALID,
 .M_EAPSC1_SUCC_CNT_ID = INVALID,
 .M_EAPSC1_FAIL_CNT_ID = INVALID,
 .M_EAPSP1_SUCC_CNT_ID = INVALID,
 .M_EAPSP1_FAIL_CNT_ID = INVALID,
 .M_EAPFC_SUCC_CNT_ID = INVALID,
 .M_EAPFC_FAIL_CNT_ID = INVALID,
 .M_EAPFC1_SUCC_CNT_ID = INVALID,
 .M_EAPFC1_FAIL_CNT_ID = INVALID,
 .M_SAS_RESP_AI_BLK_ID = INVALID,
 .M_DTIM_MBSS_BLK_ID = INVALID,
 .M_TIM_OFF_MBSS_BLK_ID = INVALID,
 .M_SAS_RESP_AI_IDX_ID = INVALID,
 .M_SAS_RESP_AI_BLK_PTR_ID = INVALID,
 .M_SAS_DBG_4_ID = INVALID,
 .M_SAS_DBG_5_ID = INVALID,
 .M_SASAIFIFO_OFLO_CNT_ID = INVALID,
 .M_TXQ_PRUNE_TABLE_ID = INVALID,
 .M_FFT_SMPL_FFT_GAIN_RX0_ID = INVALID,
 .M_FFT_SMPL_FFT_GAIN_RX1_ID = INVALID,
 .M_FFT_SMPL_FFT_GAIN_RX2_ID = INVALID,
 .M_FFT_SMPL_CTRL_ID = INVALID,
 .M_FFT_SMPL_TS_ID = INVALID,
 .M_FFT_SMPL_FRMCNT_ID = INVALID,
 .M_FFT_SMPL_RX_CHAIN_ID = INVALID,
 .M_SASAIFIFO_RPTR_ID = INVALID,
 .M_SASAIFIFO_WPTR_ID = INVALID,
 .M_FFT_SMPL_WLAN_GAIN_RX0_ID = INVALID,
 .M_FFT_SMPL_WLAN_GAIN_RX1_ID = INVALID,
 .M_FFT_SMPL_WLAN_GAIN_RX2_ID = INVALID,
 .M_FFT_SMPL_STATUS_ID = INVALID,
 .M_FFT_SMPL_SEQUENCE_NUM_ID = INVALID,
 .M_FFT_CRS_GLITCH_CNT_ID = INVALID,
 .M_FAST_NOISE_TS_ID = INVALID,
 .M_FAST_NOISE_DIFF_ID = INVALID,
 .M_SAS_FRM_RX_AI_ID = INVALID,
 .M_SAS_IMPBF_AI_ID = INVALID,
 .M_FAST_NOISE_FORCE_ID = INVALID,
 .M_FFT_SMPL_CHANNEL_ID = INVALID,
 .M_FFT_SMPL_FFT_GAIN_RX3_ID = INVALID,
 .M_FAST_NOISE_MEASURE_ID = INVALID,
 .M_FAST_NOISE_DUR_ID = INVALID,
 .M_SAS_DEFAULT_AI_RXD_ID = INVALID,
 .M_SAS_DEFAULT_AI_TXD_ID = INVALID,
 .M_SAS_GPIO_CLK_ID = INVALID,
 .M_SAS_GPIO_DATA_ID = INVALID,
 .M_FFT_SMPL_ENABLE_ID = INVALID,
 .M_FFT_SMPL_INTERVAL_ID = INVALID,
 .M_EAP_BLK_RESERVED_1_ID = INVALID,
 .M_EAP_BLK_RESERVED_2_ID = INVALID,
 .M_FIPS_LPB_ENC_BLK_ID = INVALID,
 .M_FIPS_LPB_MIC_BLK_ID = INVALID,
 .M_NM_ATTEMPTS_ID = INVALID,
 .M_NM_IFS_START_ID = INVALID,
 .M_NM_IFS_END_ID = INVALID,
 .M_NM_EDCRS_ID = INVALID,
 .M_NM_PKTPROC_ID = INVALID,
 .M_NM_POST_CLEAN_ID = INVALID,
 .M_NM_PKTPROC2_ID = INVALID,
 .M_NM_IFS_MID_ID = INVALID,
 .M_DIAG_FLAGS_ID = ((0x0*2)+(0x32*2)),
};
