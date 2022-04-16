/* Generated automatically */
static void zp_setup_options_table _(());
static int zp_conv_opt _((int zp_opt_num));
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
#if defined(MAP_SHARED) && defined(PROT_READ)
#define USE_MMAP 1
#endif
#endif
#ifdef USE_MMAP
static int custom_zwcstat _((char*filename,struct stat*buf));
#endif
static Eprog custom_check_dump_file _((char*file,struct stat*sbuf,char*name,int*ksh,int test_only));
static Wordcode custom_load_dump_header _((char*nam,char*name,int err));
static void readarray_usage _(());
static int zp_append_report _((const char*nam,const char*target,int target_len,const char*body,int body_len));
static HashTable zp_createhashtable _((char*name));
static Param zp_createhashparam _((char*name,int flags));
static void zp_free_sevent_node _((HashNode hn));
static int zp_has_option _((char**argv,char opt));
