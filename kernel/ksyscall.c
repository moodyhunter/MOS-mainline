// SPDX-License-Identifier: GPL-3.0-or-later

#include "lib/string.h"
#include "lib/structures/bitmap.h"
#include "mos/elf/elf.h"
#include "mos/filesystem/fs_types.h"
#include "mos/filesystem/vfs.h"
#include "mos/ipc/ipc.h"
#include "mos/locks/futex.h"
#include "mos/mm/kmalloc.h"
#include "mos/mm/mm_types.h"
#include "mos/mm/paging/paging.h"
#include "mos/mm/shm.h"
#include "mos/mos_global.h"
#include "mos/platform/platform.h"
#include "mos/printk.h"
#include "mos/syscall/decl.h"
#include "mos/tasks/process.h"
#include "mos/tasks/schedule.h"
#include "mos/tasks/task_types.h"
#include "mos/tasks/thread.h"
#include "mos/tasks/wait.h"
#include "mos/types.h"

void define_syscall(poweroff)(bool reboot, u32 magic)
{
#define POWEROFF_MAGIC MOS_FOURCC('G', 'B', 'y', 'e')
    if (magic != POWEROFF_MAGIC)
        mos_warn("poweroff syscall called with wrong magic number (0x%x)", magic);
    if (!reboot)
        platform_shutdown();
    else
        mos_warn("reboot is not implemented yet");
}

fd_t define_syscall(file_open)(const char *path, open_flags flags)
{
    if (path == NULL)
        return -1;

    file_t *f = vfs_open(path, flags);
    if (!f)
        return -1;
    return process_attach_ref_fd(current_process, &f->io);
}

bool define_syscall(file_stat)(const char *path, file_stat_t *stat)
{
    if (path == NULL || stat == NULL)
        return false;

    return vfs_stat(path, stat);
}

size_t define_syscall(io_read)(fd_t fd, void *buf, size_t count, size_t offset)
{
    if (fd < 0 || buf == NULL)
        return -1;
    if (offset)
    {
        mos_warn("offset is not supported yet");
        return -1;
    }
    io_t *io = process_get_fd(current_process, fd);
    if (!io)
        return -1;
    return io_read(io, buf, count);
}

size_t define_syscall(io_write)(fd_t fd, const void *buf, size_t count, size_t offset)
{
    if (fd < 0 || buf == NULL)
    {
        pr_warn("io_write called with invalid arguments (fd=%ld, buf=%p, count=%zd, offset=%zd)", fd, buf, count, offset);
        return -1;
    }
    if (offset)
    {
        mos_warn("offset is not supported yet");
        return -1;
    }
    io_t *io = process_get_fd(current_process, fd);
    if (!io)
    {
        pr_warn("io_write called with invalid fd %ld", fd);
        return -1;
    }
    return io_write(io, buf, count);
}

bool define_syscall(io_close)(fd_t fd)
{
    if (fd < 0)
        return false;
    process_detach_fd(current_process, fd);
    return true;
}

noreturn void define_syscall(exit)(u32 exit_code)
{
    const pid_t pid = current_process->pid;
    if (unlikely(pid == 1))
        mos_panic("init process exited with code %d", exit_code);

    process_handle_exit(current_process, exit_code);
    reschedule();
    MOS_UNREACHABLE();
}

void define_syscall(yield_cpu)(void)
{
    reschedule();
}

pid_t define_syscall(fork)(void)
{
    process_t *parent = current_process;
    process_t *child = process_handle_fork(parent);
    if (unlikely(child == NULL))
        return -1;
    return child->pid; // return 0 for child, pid for parent
}

pid_t define_syscall(exec)(const char *path, const char *const argv[])
{
    MOS_UNUSED(path);
    MOS_UNUSED(argv);
    mos_warn("exec syscall not implemented yet");
    return -1;
}

pid_t define_syscall(get_pid)(void)
{
    return current_process->pid;
}

pid_t define_syscall(get_parent_pid)(void)
{
    return current_process->parent->pid;
}

pid_t define_syscall(spawn)(const char *path, int argc, const char *const argv[])
{
    process_t *current = current_process;

    const char **new_argv = kmalloc(sizeof(uintptr_t) * (argc + 2)); // +1 for path, +1 for NULL
    if (new_argv == NULL)
        return -1;

    const int real_argc = argc + 1; // +1 for path, but not including NULL
    new_argv[0] = strdup(path);
    for (int i = 0; i < argc; i++)
        new_argv[i + 1] = strdup(argv[i]);
    new_argv[real_argc] = NULL;

    process_t *process = elf_create_process(path, current, current->terminal, (argv_t){ .argc = real_argc, .argv = new_argv });
    if (process == NULL)
        return -1;

    return process->pid;
}

tid_t define_syscall(create_thread)(const char *name, thread_entry_t entry, void *arg)
{
    thread_t *thread = thread_new(current_process, THREAD_MODE_USER, name, entry, arg);
    if (thread == NULL)
        return -1;

    return thread->tid;
}

tid_t define_syscall(get_tid)(void)
{
    return current_thread->tid;
}

noreturn void define_syscall(thread_exit)(void)
{
    thread_handle_exit(current_thread);
    reschedule();
    MOS_UNREACHABLE();
}

uintptr_t define_syscall(heap_control)(heap_control_op op, uintptr_t arg)
{
    process_t *process = current_process;

    proc_vmblock_t *block = NULL;
    for (int i = 0; i < process->mmaps_count; i++)
    {
        if (process->mmaps[i].content == VMTYPE_HEAP)
        {
            block = &process->mmaps[i];
            break;
        }
    }

    if (block == NULL)
    {
        mos_warn("heap_control called but no heap block found");
        return 0;
    }

    switch (op)
    {
        case HEAP_GET_BASE: return block->blk.vaddr;
        case HEAP_GET_TOP: return block->blk.vaddr + block->blk.npages * MOS_PAGE_SIZE;
        case HEAP_SET_TOP:
        {
            if (arg < block->blk.vaddr)
            {
                mos_warn("heap_control: new top is below heap base");
                return 0;
            }

            if (arg % MOS_PAGE_SIZE)
            {
                mos_warn("heap_control: new top is not page-aligned");
                return 0;
            }

            if (arg == block->blk.vaddr + block->blk.npages * MOS_PAGE_SIZE)
                return 0; // no change

            if (arg < block->blk.vaddr + block->blk.npages * MOS_PAGE_SIZE)
            {
                mos_warn("heap_control: shrinking heap not supported yet");
                return 0;
            }

            return process_grow_heap(process, (arg - block->blk.vaddr) / MOS_PAGE_SIZE);
        }
        case HEAP_GET_SIZE: return block->blk.npages * MOS_PAGE_SIZE;
        case HEAP_GROW_PAGES:
        {
            // arg is the number of pages to grow
            // some bad guy would pass a huge value here :)
            return process_grow_heap(process, arg);
        }
        default: mos_warn("heap_control: unknown op %d", op); return 0;
    }
}

bool define_syscall(wait_for_thread)(tid_t tid)
{
    thread_t *target = thread_get(tid);
    if (target == NULL)
        return false;

    if (target->owner != current_process)
    {
        pr_warn("wait_for_thread(%ld) from process %ld (%s) but thread belongs to process %ld (%s)", tid, current_process->pid, current_process->name, target->owner->pid,
                target->owner->name);
        return false;
    }

    if (target->state == THREAD_STATE_DEAD)
    {
        return true; // thread is already dead, no need to wait
    }

    wait_condition_t *wc = wc_wait_for_thread(target);
    reschedule_for_wait_condition(wc);
    return true;
}

bool define_syscall(futex_wait)(futex_word_t *futex, u32 val)
{
    return futex_wait(futex, val);
}

bool define_syscall(futex_wake)(futex_word_t *futex, size_t count)
{
    return futex_wake(futex, count);
}

fd_t define_syscall(ipc_create)(const char *name, size_t max_pending_connections)
{
    io_t *io = ipc_create(name, max_pending_connections);
    if (io == NULL)
        return -1;
    return process_attach_ref_fd(current_process, io);
}

fd_t define_syscall(ipc_accept)(fd_t listen_fd)
{
    io_t *server = process_get_fd(current_process, listen_fd);
    io_t *client_io = ipc_accept(server);
    if (client_io == NULL)
        return -1;
    return process_attach_ref_fd(current_process, client_io);
}

fd_t define_syscall(ipc_connect)(const char *server, size_t buffer_size)
{
    io_t *io = ipc_connect(server, buffer_size);
    if (io == NULL)
        return -1;
    return process_attach_ref_fd(current_process, io);
}

u64 define_syscall(arch_syscall)(u64 syscall, u64 arg1, u64 arg2, u64 arg3, u64 arg4)
{
    return platform_arch_syscall(syscall, arg1, arg2, arg3, arg4);
}

bool define_syscall(vfs_mount)(const char *device, const char *mountpoint, const char *fs_type, const char *options)
{
    return vfs_mount(device, mountpoint, fs_type, options);
}

ssize_t define_syscall(vfs_readlink)(const char *path, char *buf, size_t buflen)
{
    return vfs_readlink(path, buf, buflen);
}

bool define_syscall(vfs_touch)(const char *path, file_type_t type, u32 mode)
{
    return vfs_touch(path, type, mode);
}

bool define_syscall(vfs_symlink)(const char *target, const char *linkpath)
{
    return vfs_symlink(target, linkpath);
}

bool define_syscall(vfs_mkdir)(const char *path)
{
    return vfs_mkdir(path);
}

size_t define_syscall(vfs_list_dir)(fd_t fd, char *buffer, size_t buffer_size)
{
    io_t *io = process_get_fd(current_process, fd);
    if (io == NULL)
        return false;
    return vfs_list_dir(io, buffer, buffer_size);
}

bool define_syscall(vfs_fstat)(fd_t fd, file_stat_t *statbuf)
{
    io_t *io = process_get_fd(current_process, fd);
    if (io == NULL)
        return false;
    return vfs_fstat(io, statbuf);
}

void *define_syscall(mmap_anonymous)(uintptr_t hint_addr, size_t size, mem_perm_t perm, mmap_flags_t flags)
{
    const bool exact = flags & MMAP_EXACT;
    const bool shared = flags & MMAP_SHARED;   // shared when forked, otherwise
    const bool private = flags & MMAP_PRIVATE; // copy-on-write when forked

    pr_info("mmap_anonymous(" PTR_FMT ", %zd, %c%c%c, %c, %c%c)", //
            hint_addr,                                            //
            size,                                                 //
            perm & MEM_PERM_READ ? 'r' : '-',                     //
            perm & MEM_PERM_WRITE ? 'w' : '-',                    //
            perm & MEM_PERM_EXEC ? 'x' : '-',                     //
            exact ? 'E' : '-',                                    //
            shared ? 's' : '-',                                   //
            private ? 'p' : '-'                                   //
    );

    if ((shared && private) || (!shared && !private))
    {
        pr_warn("mmap_anonymous: shared and private are mutually exclusive, and one of them must be specified");
        return NULL;
    }

    const vm_flags vmflags = VM_USER | (vm_flags) perm; // vm_flags shares the same values as mem_perm_t
    const vmblock_flags_t block_flags = shared ? VMBLOCK_FORK_SHARED : VMBLOCK_FORK_PRIVATE;

    if (hint_addr == 0 && exact)
    {
        // WTF is this? Trying to map at address 0?
        pr_warn("mmap_anonymous: trying to map at address 0");
        return NULL;
    }
    else if (hint_addr != 0 && !exact)
    {
        // TODO: deduce mapping based on an address
        pr_warn("mmap_anonymous: deduced mapping based on an address is not supported yet");
        return NULL;
    }
    else if (hint_addr == 0 && !exact)
    {
        // the kernel will choose the address
        pr_info2("mmap_anonymous: the kernel will choose the address");
        vmblock_t block = mm_alloc_pages(current_process->pagetable, ALIGN_DOWN_TO_PAGE(size) / MOS_PAGE_SIZE, PGALLOC_HINT_MMAP, vmflags);
        if (block.npages == 0)
        {
            pr_warn("mmap_anonymous: failed to allocate memory");
            return NULL;
        }

        pr_info2("mmap_anonymous: allocated %zd pages at " PTR_FMT, block.npages, block.vaddr);
        process_attach_mmap(current_process, block, VMTYPE_MMAP, block_flags);
        return (void *) block.vaddr;
    }
    else if (hint_addr != 0 && exact)
    {
        // the kernel will map at the exact address (if it's available)
        pr_info2("mmap_anonymous: the kernel will map at the exact address " PTR_FMT, hint_addr);
        vmblock_t block = mm_alloc_pages_at(current_process->pagetable, hint_addr, ALIGN_DOWN_TO_PAGE(size) / MOS_PAGE_SIZE, vmflags);
        if (block.npages == 0)
        {
            pr_warn("mmap_anonymous: failed to allocate memory");
            return NULL;
        }

        pr_info2("mmap_anonymous: allocated %zd pages at " PTR_FMT, block.npages, block.vaddr);
        process_attach_mmap(current_process, block, VMTYPE_MMAP, block_flags);
        return (void *) block.vaddr;
    }
    else
    {
        MOS_UNREACHABLE();
    }

    return NULL;
}

void *define_syscall(mmap_file)(uintptr_t hint_addr, size_t size, mem_perm_t perm, mmap_flags_t flags, fd_t fd, off_t offset)
{
    pr_info("mmap_file(" PTR_FMT ", %zd, %d, %d, %ld, %ld)", hint_addr, size, perm, flags, fd, offset);
    return NULL;
}
