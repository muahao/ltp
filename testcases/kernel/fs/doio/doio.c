#include "pattern.h"

#define	NMEMALLOC	32
#define	MEM_DATA	1	/* data space 				*/
#define	MEM_SHMEM	2	/* System V shared memory 		*/
#define	MEM_T3ESHMEM	3	/* T3E Shared Memory 			*/
#define	MEM_MMAP	4	/* mmap(2) 				*/

#define	MEMF_PRIVATE	0001
#define	MEMF_AUTORESRV	0002
#define	MEMF_LOCAL	0004
#define	MEMF_SHARED	0010

#define	MEMF_FIXADDR	0100
#define	MEMF_ADDR	0200
#define	MEMF_AUTOGROW	0400
#define	MEMF_FILE	01000	/* regular file -- unlink on close	*/
#define	MEMF_MPIN	010000	/* use mpin(2) to lock pages in memory */

struct memalloc {
	int	memtype;
	int	flags;
	int	nblks;
	char	*name;
	void	*space;		/* memory address of allocated space */
	int	fd;		/* FD open for mmaping */
	int	size;
}	Memalloc[NMEMALLOC];

/*
 * Structure for maintaining open file test descriptors.  Used by
 * alloc_fd().
 */

struct fd_cache {
	char    c_file[MAX_FNAME_LENGTH+1];
	int	c_oflags;
	int	c_fd;
	long    c_rtc;
#ifdef sgi
	int	c_memalign;	/* from F_DIOINFO */
	int	c_miniosz;
	int	c_maxiosz;
#endif
#ifndef CRAY
	void	*c_memaddr;	/* mmapped address */
	int	c_memlen;	/* length of above region */
#endif
};

/*
 * Name-To-Value map
 * Used to map cmdline arguments to values
 */
struct smap {
	char    *string;
	int	value;
};

struct aio_info {
	int			busy;
	int			id;
	int			fd;
	int			strategy;
	volatile int		done;
#ifdef CRAY
	struct iosw		iosw;
#endif
#ifdef sgi
	aiocb_t			aiocb;
	int			aio_ret;	/* from aio_return */
	int			aio_errno;	/* from aio_error */
#endif
	int			sig;
	int			signalled;
	struct sigaction	osa;
};

/* ---------------------------------------------------------------------------
 *
 * A new paradigm of doing the r/w system call where there is a "stub"
 * function that builds the info for the system call, then does the system
 * call; this is called by code that is common to all system calls and does
 * the syscall return checking, async I/O wait, iosw check, etc.
 *
 * Flags:
 *	WRITE, ASYNC, SSD/SDS,
 *	FILE_LOCK, WRITE_LOG, VERIFY_DATA,
 */

struct	status {
	int	rval;		/* syscall return */
	int	err;		/* errno */
	int	*aioid;		/* list of async I/O structures */
};

struct syscall_info {
	char		*sy_name;
	int		sy_type;
	struct status	*(*sy_syscall)();
	int		(*sy_buffer)();
	char		*(*sy_format)();
	int		sy_flags;
	int		sy_bits;
};

#define	SY_WRITE		00001
#define	SY_ASYNC		00010
#define	SY_IOSW			00020
#define	SY_SDS			00100
char		*syserrno(int err);
void		doio(void);
void		doio_delay(void);
char		*format_oflags(int oflags);
char		*format_strat(int strategy);
char		*format_rw(struct io_req *ioreq, int fd, void *buffer,
			int signo, char *pattern, void *iosw);
#ifdef CRAY
char		*format_sds(struct io_req *ioreq, void *buffer, int sds
			char *pattern);
#endif /* CRAY */

int		do_read(struct io_req *req);
int		do_write(struct io_req *req);
int		lock_file_region(char *fname, int fd, int type, int start,
			int nbytes);

#ifdef CRAY
char		*format_listio(struct io_req *ioreq, int lcmd,
			struct listreq *list, int nent, int fd, char *pattern);
#endif /* CRAY */

int		do_listio(struct io_req *req);

#if defined(_CRAY1) || defined(CRAY)
int		do_ssdio(struct io_req *req);
#endif /* defined(_CRAY1) || defined(CRAY) */

char		*fmt_ioreq(struct io_req *ioreq, struct syscall_info *sy,
			int fd);

#ifdef CRAY
struct status	*sy_listio(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
int		listio_mem(struct io_req *req, int offset, int fmstride,
			int *min, int *max);
char		*fmt_listio(struct io_req *req, struct syscall_info *sy,
			int fd, char *addr);
#endif /* CRAY */

#ifdef sgi
struct status	*sy_pread(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_pwrite(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
char		*fmt_pread(struct io_req *req, struct syscall_info *sy,
			int fd, char *addr);
#endif	/* sgi */
#ifndef CRAY
struct status	*sy_readv(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_writev(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_rwv(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr, int rw);
char		*fmt_readv(struct io_req *req, struct syscall_info *sy,
			int fd, char *addr);
#endif /* !CRAY */

#ifdef sgi
struct status	*sy_aread(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_awrite(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr)
struct status	*sy_arw(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr, int rw);
char		*fmt_aread(struct io_req *req, struct syscall_info *sy,
			int fd, char *addr);
#endif /* sgi */
struct status	*sy_mmread(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_mmwrite(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr);
struct status	*sy_mmrw(struct io_req *req, struct syscall_info *sysc,
			int fd, char *addr, int rw);
char		*fmt_mmrw(struct io_req *req, struct syscall_info *sy, int fd,
			char *addr);
#endif /* !CRAY */
int		do_rw(struct io_req *req);

#ifdef sgi
int		do_fcntl(struct io_req *req);
#endif /* sgi */

#ifndef CRAY
int		do_sync(struct io_req *req);
#endif /* !CRAY */

int		doio_pat_fill(char *addr, int mem_needed, char *Pattern,
			int Pattern_Length, int shift);
char		*doio_pat_check(char *buf, int offset, int length,
			char *pattern, int pattern_length, int patshift);
char		*check_file(char *file, int offset, int length, char *pattern,
			int pattern_length, int patshift, int fsa);
int		doio_fprintf(FILE *stream, char *format, ...);
int		alloc_mem(int nbytes);

#if defined(_CRAY1) || defined(CRAY)
int		alloc_sds(int nbytes);
#endif /* defined(_CRAY1) || defined(CRAY) */

int		alloc_fd(char *file, int oflags);
struct fd_cache	*alloc_fdcache(char *file, int oflags);

#ifdef sgi
void		signal_info(int sig, siginfo_t *info, void *v);
void		cleanup_handler(int sig, siginfo_t *info, void *v);
void		die_handler(int sig, siginfo_t *info, void *v);
void		sigbus_handler(int sig, siginfo_t *info, void *v);
#else	/* !sgi */
void		cleanup_handler(int sig);
void		die_handler(int sig);

#ifndef CRAY
void		sigbus_handler(int sig);
#endif /* !CRAY */
#endif /* sgi */

void		noop_handler(int sig);
void		sigint_handler(int sig);
void		aio_handler(int sig);
void		dump_aio(void);

#ifdef sgi
void		cb_handler(sigval_t val);
#endif /* sgi */

struct aio_info	*aio_slot(int aio_id);
int		aio_register(int fd, int strategy, int sig);
int		aio_unregister(int aio_id);

#ifndef __linux__
int		aio_wait(int aio_id);
#endif /* !__linux__ */

char		*hms(time_t t);
int		aio_done(struct aio_info *ainfo);
void		doio_upanic(int mask);
int		parse_cmdline(int argc, char **argv, char *opts);

#ifndef CRAY
void		parse_memalloc(char *arg);
void		dump_memalloc(void);
#endif /* !CRAY */

void		parse_delay(char *arg);
int		usage(FILE *stream);
void		help(FILE *stream);
main(int argc, char **argv)
doio(void)
doio_delay(void)
format_rw(struct io_req *ioreq, int fd, void *buffer, int signo, char *pattern,
	void *iosw)
format_sds(struct io_req *ioreq, void *buffer, int sds, char *pattern)
do_read(struct io_req *req)
do_write(struct io_req *req)
lock_file_region(char *fname, int fd, int type, int start, int nbytes)
format_listio(struct io_req *ioreq, int lcmd, struct listreq *list,
	int nent, int fd, char *pattern)
do_listio(struct io_req *req)
do_ssdio(struct io_req *req)
do_ssdio(struct io_req *req)
#endif /* CRAY */
sy_listio(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
listio_mem(struct io_req *req, int offset, int fmstride, int *min, int *max)
sy_pread(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_pwrite(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_readv(struct io_req	*req, struct syscall_info *sysc, int fd, char *addr)
sy_writev(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_rwv(struct io_req *req, struct syscall_info *sysc, int fd, char *addr,
	int rw)
sy_aread(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_awrite(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_arw(struct io_req *req, struct syscall_info *sysc, int fd, char *addr,
	int rw)
sy_mmread(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_mmwrite(struct io_req *req, struct syscall_info *sysc, int fd, char *addr)
sy_mmrw(struct io_req *req, struct syscall_info *sysc, int fd, char *addr,
	int rw)
do_rw(struct io_req *req)
do_fcntl(struct io_req *req)
#endif /* sgi */
do_sync(struct io_req *req)
#endif /* !CRAY */
	int shift)
doio_pat_check(char *buf, int offset, int length, char *pattern,
	int pattern_length, int patshift)
check_file(char *file, int offset, int length, char *pattern,
	int pattern_length, int patshift, int fsa)
alloc_mem(int nbytes)
#else /* CRAY */
alloc_mem(int nbytes)
alloc_sds(int nbytes)
alloc_sds(int nbytes)
alloc_fd(char *file, int oflags)
alloc_fdcache(char *file, int oflags)
cleanup_handler(int sig)
die_handler(int sig)
sigbus_handler(int sig)
		cleanup_handler(sig);
noop_handler(int sig)
sigint_handler(int sig)
aio_handler(int sig)
dump_aio(void)
cb_handler(sigval_t val)
aio_slot(int aio_id)
aio_register(int fd, int strategy, int sig)
aio_unregister(int aio_id)
aio_wait(int aio_id)
hms(time_t t)
doio_upanic(int mask)
parse_cmdline(int argc, char **argv, char *opts)
dump_memalloc(void)
usage(FILE *stream)
help(FILE *stream)
}