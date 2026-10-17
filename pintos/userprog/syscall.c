#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *); // register handler

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081					/* Segment selector msr */
#define MSR_LSTAR 0xc0000082				/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
													((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
						FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

static void is_kernel_range(uint64_t addr, size_t size)
{
	if (size == 0)
	{
		uint64_t next_addr = addr + 1;
		ASSERT(is_kernel_vaddr(next_addr));
		unsigned char value = *(unsigned char *)next_addr;

		while (value != '\0')
		{
			if (is_kernel_vaddr(next_addr))
			{
				sys_exit(-1);
			}
			next_addr += 1;
			value = *(unsigned char *)next_addr;
		}
	}
	else
	{
		uint64_t end_addr = addr + size;
		if (is_kernel_vaddr(addr) || is_kernel_vaddr(end_addr))
		{
			sys_exit(-1);
		}
	}
}

static void crosses_page_boundary(uint64_t addr, size_t size)
{

	uint64_t end_addr = addr + size;
	uint64_t start_pg_no = pg_no(pg_round_down(addr));
	uint64_t end_pg_no = pg_no(pg_round_down(end_addr));

	struct thread *t = thread_current();
	uint64_t *pml4 = t->pml4;

	if (start_pg_no == end_pg_no)
	{
		if (pml4_get_page(pml4, pg_addr(start_pg_no)) == NULL)
		{
			sys_exit(-1);
		}
	}
	else
	{
		while (start_pg_no <= end_pg_no)
		{
			if (pml4_get_page(pml4, pg_addr(start_pg_no)) == NULL)
			{
				sys_exit(-1);
			}
			start_pg_no += 1;
		}
	}
}

static void check_address_range(uint64_t addr, size_t size)
{
	is_kernel_range(addr, size);
	crosses_page_boundary(addr, size);
}

static void isAddressValid(struct intr_frame *f UNUSED)
{
	switch (f->R.rax)
	{
	case SYS_FORK:
		check_address_range(f->R.rdi, sizeof(char));
		// pid_t fork (const char *thread_name);
		break;
	case SYS_EXEC:
		check_address_range(f->R.rdi, sizeof(char));
		// int exec (const char *file);
		break;
	case SYS_CREATE:
		check_address_range(f->R.rdi, sizeof(char));
		// bool create (const char *file, unsigned initial_size);
		break;
	case SYS_REMOVE:
		check_address_range(f->R.rdi, sizeof(char));
		// bool remove (const char *file);
		break;
	case SYS_OPEN:
		check_address_range(f->R.rdi, sizeof(char));
		// int open (const char *file);
		break;
	case SYS_READ:
		check_address_range(f->R.rsi, f->R.rdx);
		// int read (int fd, void *buffer, unsigned length);
		break;
	case SYS_WRITE:
		check_address_range(f->R.rsi, f->R.rdx);
		// int write (int fd, const void *buffer, unsigned length);
		break;
	case SYS_MMAP:
		check_address_range(f->R.rdi, f->R.rsi);
		// void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
		break;
	case SYS_MUNMAP:
		check_address_range(f->R.rdi, sizeof(void));
		// void munmap (void *addr);
		break;
	case SYS_CHDIR:
		check_address_range(f->R.rdi, sizeof(char));
		// bool chdir (const char *dir);
		break;
	case SYS_MKDIR:
		check_address_range(f->R.rdi, sizeof(char));
		// bool mkdir (const char *dir);
		break;
	case SYS_SYMLINK:
		check_address_range(f->R.rdi, sizeof(char));
		check_address_range(f->R.rsi, sizeof(char));
		// int symlink (const char* target, const char* linkpath);
		break;

	default:
		break;
	}
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED)
{

	// TODO: Your implementation goes here.
	printf("system call!\n");
	thread_exit();
}
