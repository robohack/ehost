/*
** Declaration of functions.
**
**	@(#)defs.h              e07@nikhef.nl (Eric Wassenaar) 991529
*/

#ident "@(#)host:$Name:  $:$Id: defs.h,v 1.4 2003-03-28 21:57:23 -0800 woods Exp $"

/*
** Internal modules of the host utility
** ------------------------------------
*/
	/* main.c */

int main		__P((int, char **));
void set_defaults	__P((char *, int, char **));
int getval		__P((char *, char *, int, int));
char *cvtopt		__P((char *));
int process_argv	__P((int, char **));
int process_file	__P((FILE *));
int process_name	__P((char *));
int execute_name	__P((char *));
bool_t execute		__P((char *, ipaddr_t));
bool_t host_query	__P((char *, ipaddr_t));
char *myhostname	__P((void));
void set_server		__P((char *));
void set_logfile	__P((char *));
void set_cachedir	__P((char *));
void fatal		__P((char *, ...));
void errmsg		__P((char *, ...));

	/* info.c */

bool_t get_hostinfo	__P((char *, bool_t));
bool_t get_domaininfo	__P((char *, char *));
int get_info		__P((querybuf_t *, const char *, int, int));
bool_t print_info	__P((querybuf_t *, int, const char *, int, int, bool_t));
void print_data		__P((char *, ...));
u_char *print_rrec	__P((const char *, int, int, u_char *, u_char *, u_char *, bool_t));
void dump_rrec		__P((u_char *, int, char *));
u_char *skip_qrec	__P((const char *, int, int, u_char *, u_char *, u_char *));
bool_t get_recursive	__P((char **));

	/* list.c */

bool_t list_zone	__P((char *));
bool_t use_servers	__P((char *));
bool_t find_servers	__P((char *));
bool_t get_servers	__P((char *));
bool_t get_nsinfo	__P((querybuf_t *, int, char *, int, int));
void sort_servers	__P((void));
bool_t skip_transfer	__P((char *));
void do_check		__P((char *));
void do_soa		__P((char *, struct in_addr, char *));
bool_t do_transfer	__P((char *));
bool_t transfer_zone	__P((char *, struct in_addr, char *));
bool_t get_zone		__P((char *, struct in_addr, char *));
void update_zone	__P((const char *));
bool_t get_mxrec	__P((char *));
char *get_primary	__P((char *));
bool_t check_zone	__P((char *, char *));
bool_t check_cache	__P((char *, char *));
bool_t compare_soa	__P((char *));
bool_t get_soainfo	__P((querybuf_t *, int, char *, int, int));
int load_soa		__P((querybuf_t *, char *));
void check_soa		__P((querybuf_t *, char *, char *));
bool_t check_dupl	__P((ipaddr_t));
bool_t check_ttl	__P((char *, int, int, int));
void clear_ttltab	__P((void));
int host_index		__P((char *, bool_t));
void clear_hosttab	__P((void));
int zone_index		__P((char *, bool_t));
void clear_zonetab	__P((void));
int check_canon		__P((char *));

	/* addr.c */

bool_t check_addr	__P((char *));
bool_t check_addr_name	__P((struct in_addr, char *));
bool_t check_name	__P((ipaddr_t));
bool_t check_name_addr	__P((char *, ipaddr_t));

	/* geth.c */

struct hostent *geth_byname	__P((const char *));
struct hostent *geth_byaddr	__P((const char *, size_t, int));

	/* util.c */

int parse_type		__P((char *));
int parse_class		__P((char *));
char *in_addr_arpa	__P((char *));
char *nsap_int		__P((char *));
void print_host		__P((char *, struct hostent *));
void show_res		__P((void));
void print_stats	__P((int *, int, char *, int, int));
void clear_stats	__P((int *));
void show_types		__P((char *, int, int));
void ns_error		__P((char *, int, int, char *));
char *decode_error	__P((int));
void print_answer	__P((querybuf_t *, int, int));
void pr_error		__P((char *, ...));
void pr_warning		__P((char *, ...));
void pr_timestamp	__P((char *, ...));
bool_t want_type	__P((int, int));
bool_t want_class	__P((int, int));
bool_t indomain		__P((char *, const char *, bool_t));
bool_t samedomain	__P((char *, char *, bool_t));
bool_t gluerecord	__P((char *, char *, char **, int));
int matchlabels		__P((char *, char *));
char *pr_domain		__P((char *, bool_t));
char *pr_dotname	__P((char *));
char *pr_nsap		__P((char *));
char *pr_type		__P((int));
char *pr_class		__P((int));
int expand_name		__P((const char *, int, u_char *, u_char *, u_char *, char *));
int check_size		__P((char *, int, u_char *, u_char *, u_char *, int));
bool_t valid_name	__P((char *, bool_t, bool_t, bool_t));
int canonical		__P((char *));
char *mapreverse	__P((char *, struct in_addr));
int anyrecord		__P((char *));
int compare_name	__P((const ptr_t *, const ptr_t *));

	/* misc.c */

ptr_t *xalloc		__P((ptr_t *, size_t));
char *dtoa		__P((int));
char *utoa		__P((int));
char *xtoa		__P((int));
char *stoa		__P((u_char *, int, bool_t));
char *base_ntoa		__P((u_char *, int));
char *nsap_ntoa		__P((u_char *, int));
char *ipng_ntoa		__P((u_char *));
char *pr_date		__P((int));
char *pr_time		__P((int, bool_t));
char *pr_spherical	__P((int, char *, char *));
char *pr_vertical	__P((int, char *, char *));
char *pr_precision	__P((int));
int convtime		__P((char *, int));

	/* test.c */

bool_t test		__P((char *, ipaddr_t));

	/* file.c */

char *cachename		__P((char *, char *, int));
int cache_open		__P((char *, bool_t));
int cache_close		__P((bool_t));
int cache_write		__P((char *, size_t));
int cache_read		__P((char *, size_t));
void cache_perror	__P((char *, char *));

	/* send.c */

#ifdef HOST_RES_SEND
int res_send		__P((const qbuf_t *, int, qbuf_t *, int));
void _res_close		__P((void));
static bool_t check_from __P((void));
static int send_stream	__P((struct sockaddr_in *, qbuf_t *, int, qbuf_t *, int));
static int send_dgram	__P((struct sockaddr_in *, qbuf_t *, int, qbuf_t *, int));
#endif /*HOST_RES_SEND*/
int _res_socket		__P((int, int, int));
int _res_blocking	__P((int, bool_t));
sigtype_t timer		__P((int));
int _res_connect	__P((int, struct sockaddr_in *, size_t));
int _res_write		__P((int, struct sockaddr_in *, char *, char *, size_t));
int _res_read		__P((int, struct sockaddr_in *, char *, char *, size_t));
int _res_read_stream	__P((int, struct sockaddr_in *, char *, char *, size_t));
int recv_sock		__P((int, char *, size_t));
void _res_perror	__P((struct sockaddr_in *, char *, char *));

/*
** External library functions
** --------------------------
*/
	/* extern */

time_t time		__P((time_t *));

#if !defined(NO_INET_H)
# include <arpa/inet.h>
#else

ipaddr_t inet_addr	__P((const char *));
char *inet_ntoa		__P((struct in_addr));

#endif

#if defined(BIND_48)
char *hostalias		__P((const char *));
#endif

	/* avoid <strings.h> */

#if !defined(index)

char *index		__P((const char *, int));
char *rindex		__P((const char *, int));

#endif

	/* <string.h> */

#if !defined(NO_STRING_H)
# include <string.h>
#else

char *strcpy		__P((char *, const char *));
char *strncpy		__P((char *, const char *, size_t));

#endif

	/* <stdlib.h> */

#if defined(__STDC__) && !defined(apollo)
# include <stdlib.h>
#else

char *getenv		__P((const char *));
ptr_t *malloc		__P((size_t));
ptr_t *realloc		__P((ptr_t *, size_t));
free_t free		__P((ptr_t *));
void exit		__P((int));
void qsort		__P((ptr_t *, size_t, size_t, int (*) __P((const ptr_t *, const ptr_t *))));

#endif

	/* <unistd.h> */

#if defined(__STDC__) && !defined(apollo) && !defined(WINNT)
# include <unistd.h>
#else

unsigned int alarm	__P((unsigned int));

#endif
