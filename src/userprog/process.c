#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE); /* fn_copy 에 file_name 을 복사, 단 PGSIZE 사이즈를 넘으면 안됨. */

  /* Create a new thread to execute FILE_NAME. */

  /* REMOVE
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  */

  // PLAN
  // file_name 의 첫 인자가 진짜 file name 이다.
  char* dummyptr;
  char* token = strtok_r(file_name, " ", &dummyptr); // 여기에 &save_ptr 대신 NULL을 넣어도 무방함. save_ptr은 이후 안쓰임. 함 해보까?
  // >> 여기에 NULL 넣어줬더니 kernel PANIC 떠서 dummyptr 해줌.

  // MYCODE_START
//printf(">> in process_execute, token: %s\n", token);
  if (filesys_open (token) == NULL)
    return -1;
  // MYCODE_END

  struct thread *current = thread_current();
  tid = thread_create (token, PRI_DEFAULT, start_process, fn_copy);
  sema_down (&current->load_lock);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  
  struct list_elem* iter = NULL;
  struct thread *elem = NULL;
  for (iter = list_begin(&(current->children)); iter != list_end(&(current->children)); iter = list_next(iter))
  {
    elem = list_entry (iter, struct thread, child_elem);
    if (elem->exit_code == -1)
      return process_wait (tid);
  }
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
//printf(" >> start_process() start!\n");
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG; // User data selector(뭔가 단위인듯)
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  /*
    여기에서 file_name parsing 하고, load의 인자인 file_name에 파일명만 넣기
  */





////////////////// strtok start ////////////////////
// input: char *file_name

//printf("    >> MYCODE_START\n");
  // MYCODE_START
  // using strtok_r reference: https://codeday.me/ko/qa/20190508/495336.html
  char *ptr; // make q point to start of file_name.
  char *rest; // to point to the rest of the string after token extraction.
  char *token; // to point to the actual token returned.

  /* init cpy_file_name for calculating argc. */
  char *cpy_file_name = (char *)malloc (sizeof (file_name));
  strlcpy (cpy_file_name, file_name, strlen(file_name) + 1);

  ptr = cpy_file_name;

  /*
  argv[0] = prog_name
  argv[1] = 1st arg
  argv[2] = 2nd arg
  ...
  */
   
  char **argv;
  int argc = 0;
  /* Get argc's length. */
//printf("  >> Get argc's length; while loop.\n");
  token = strtok_r (ptr, " ", &rest);
//printf("    >> obtd token: %s\n", token);
//printf("       in argc: %d\n", argc);
  argc ++;
  ptr = rest;
  while (token != NULL)
  {
    token = strtok_r (ptr, " ", &rest);
//printf("    >> obtd token: %s\n", token);
//printf("       in argc: %d\n", argc);
    argc ++;
    ptr = rest;
  }
  argc --;
//printf("    >> summery argc: %d\n", argc);
  free (cpy_file_name);

  argv = (char **)malloc(sizeof(char *) * argc);

  ptr = file_name;
  // loop untill strtok_r return NULL
   
  int i = 0;
  token = strtok_r (ptr, " ", &rest);
  argv[i] = token;
//printf("      >> saved argv: %s\n", argv[i]);
//printf("      >> i: %d\n", i);
  i ++;
  ptr = rest;
  while (i != argc)
  {
    token = strtok_r (ptr, " ", &rest);
    argv[i] = token;
//printf("      >> saved argv: %s\n", argv[i]);
//printf("      >> i: %d\n", i);
    i ++;
    ptr = rest;
  }
  // MYCODE_END
//printf("    >> MYCODE_END\n");

// output: char **argv, int argc
////////////////// strtok end ////////////////////





  success = load (argv[0], &if_.eip, &if_.esp);
//printf(" >> in start_process(), load() returns true!\n");






  if (success)
  {
    //  push_to_esp (&if_.esp, &file_name, &&argv, argc);
    void **esp = &if_.esp;
//printf(" >> push_to_esp invoked!\n");
//printf("  >> passed argc: %d\n", argc);

    /* push command line (in argv) value.

        ls      -l      foo      bar
      argv[0]   [1]     [2]      [3]

      these will be pushed in right-to-left order.
      each size is (strlen (argv[i])) + 1
    */

    int length = 0;
//printf("  >> for loop pushing argv execute.\n");
    for (int i = argc - 1; i >= 0; i--)
    {
//printf("  >> i: %d\n", i);
      length = strlen (argv[i]) + 1; // '\n'도 넣기 위해 +1
//printf("  >> length of argv[i]: %d\n", length);
      *esp -= length;
//printf("      >> extract by: %d\n", length + 1);
      memcpy (*esp, argv[i], length);
      // strlcpy (*esp, argv[i], length + 1);
      argv[i] = *esp;
    }

//printf("  >> push command line finished / push word-align start\n");
    /* push word-align. */
    while ( (PHYS_BASE - *esp) % 4 != 0 ){
//printf("      >> PHYS_BASE - *esp = %d\n", PHYS_BASE - *esp);
//printf("      >> , so we extract stack %d\n", sizeof (uint8_t));
//printf("      >> , and push 0.\n");
      *esp -= sizeof (uint8_t);
      **(uint8_t **)esp = 0;
    }

//printf("  >> push word-align finished / push NULL start\n");

    /* push NULL */
    *esp -= 4;
    *(uint8_t *)*esp = 0;

//printf("  >> push NULL finished / push address of argv[i] start\n");

    /* push address of argv[i]. */
    for (int i = argc - 1; i >= 0; i--)
    {
      *esp -= sizeof (uint32_t **);
//printf("      >> extract by: %d\n", sizeof (uint32_t **));
      *(uint32_t **)*esp = argv[i];
    }

//printf("  >> push address of argv[i] finished / push address of argv start\n");

    /* push address of argv. */
    *esp -= sizeof (uint32_t **);
//printf("      >> extract by: %d\n", sizeof (uint32_t **));
    *(uint32_t *)*esp = *esp + 4;

//printf("  >> push address of argv finished / push the value of argc start\n");

    /* push the value of argc. */
    *esp -= sizeof (uint32_t);
//printf("      >> extract by: %d\n", sizeof (uint32_t));
    *(uint32_t *)*esp = argc;

//printf("  >> push the value of argc finished / push return address start\n");

    /* push return address. */
    // 리턴어드레스의 크기는 4란다.
    *esp -= 4;
    *(uint32_t *)*esp = 0;
//printf("  >> push return address finished / free(argv) start\n");

// hex_dump (*esp, *esp, 100, 1);  
    free (argv);
//printf(" >> push_to_esp end!\n");
// MYCODE_END
  }





  /* If load failed, quit. */
  palloc_free_page (file_name);
  sema_up (&thread_current()->parent->load_lock);
  if (!success) 
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
//printf(" >> in start_process(), invoking asm volatile()... \n");
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
//printf(" >> in start_process(), asm volatile() finished! \n ");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid) 
{
  // MYCODE_START
//printf("  >> invoking process_wait () !\n");
  struct thread *current = thread_current();
  struct list_elem* iter = NULL;
  struct thread *elem = NULL;
  int return_value;
      /*
      이를 구현하기 위해, 현재 프로세스(스레드)는 "thread/synch.h"에 정의된
      void cond_wait (struct condition *, struct lock *)를 통해 자식프로세스의 종료를 기다리고,
      반대로 자식프로세스는 void cond_signal (struct condition *, struct lock *)
      를 통해 종료를 알리게 한다.
       --> 세마포어로 구현함
      */
/*
  if (child->tid != child_tid)
  {
//printf("  >> this pid is not a direct child of current process! return -1\n");
    return -1;
  }
  else
  {
    if (child->status == THREAD_DYING)
    {
//printf("  >> this pid was terminated by KERNEL!\n");
// or
//printf("  >> this pid was already terminated by parent!\n");
      return -1;
    }
    else if (child->status == THREAD_BLOCKED) // 이거 조건 부정확함!!!!!!
    {
//printf("  >> this pid is already waited!\n");
      return -1;
    }
    else // SUCCESS*/
    
  for (iter = list_begin(&(current->children)); iter != list_end(&(current->children)); iter = list_next(iter))
  {
    elem = list_entry (iter, struct thread, child_elem);
    if (elem->tid == child_tid)
    {
      sema_down (&(elem->child_lock));
      return_value = elem->exit_code;
      list_remove (&(elem->child_elem));
      sema_up (&(elem->memory_lock));
      return return_value;
    }
  }
  return -1;

/* // 스레드가 직속 자식만 포인트할 때
  sema_down (&(child->child_lock)); // wait
  int exit_code = child->exit_code;
  child->child = NULL; // remove
  sema_up (&(child->memory_lock)); // send signal to the parent
  return exit_code;
*/
    // MYCODE_END
}
  
  /*
  int i=0;
  int j=0;
  for (i=0; i<1000000000; i++)
    j ++;

  return -1;
  */


/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
    // MYCODE_START
    sema_up (&(cur->child_lock));
    sema_down (&(cur->memory_lock));
    // MYCODE_END
//printf("    >> process_exit() complete\n!");
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{ 
//printf(" >> load() start!\n");
//printf("   >> *file_name = %s\n", file_name);
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  /* Open executable file. */
//printf("    >> argv[0]'s size: %d\n", sizeof (file_name));
  // lock_acquire (&file_lock);
  file = filesys_open (file_name);
  if (file == NULL) 
    {
//printf ("load: %s: open failed\n", file_name);
      // lock_release (&file_lock);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (int i = 0; i < ehdr.e_phnum; i++) 
    {
//printf("  >> inside for? \n" );
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
//printf(" >> header is not 0 \n");
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
//printf(" >> header is 0 \n");
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable)){
//printf(" >> load_segment() failed! \n");
                // lock_release (&file_lock);
                goto done;
                                 }
            }
          else{
//printf(" >> validate_segment() return false!\n ");
            // lock_release (&file_lock);
            goto done;
          }
          break;
        }
    }


 // MYCODE_START
//printf("MYCODE_START ; invoking setup_stack()... \n");
  // set up stack
  if (!setup_stack (esp)){
//printf("MYCODE_END ; setup_stack() returns false \n");
    goto done;
  }
//printf("MYCODE_END ; setup_stack() returns success! \n");
 // MYCODE_END

  /* Start address. */

  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
//printf("  >> invoking file_cloes (file) ... \n");
  file_close (file);
//printf("  >> file_cloes (file) clear. load() returns success! \n");
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)){ 
//printf("    ! validate_segment() false: 0 \n");
    return false; 
  }

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)){ 
//printf("    ! validate_segment() false: 1 \n");
    return false; 
  }

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz){ 
//printf("    ! validate_segment() false: 2 \n");
    return false; 
  }

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0){ 
//rintf("    ! validate_segment() false: 3 \n");
    return false; 
  }
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr)){ 
//printf("    ! validate_segment() false: 4 \n");
    return false; 
  }
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz))){ 
printf("    ! validate_segment() false: 5 \n");
    return false; 
  }
  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr){ 
//printf("    ! validate_segment() false: 6 \n");
    return false; 
  }

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE){ 
//printf("    ! validate_segment() false: 7 \n");
    return false; 
  }

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
//printf(" >> setup_stack() invoked! \n");
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success){
        *esp = PHYS_BASE; // initialize sp
      }
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
