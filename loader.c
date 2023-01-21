#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "exec_parser.h"

static so_exec_t *exec;
static int pageSize;
static int fd;

int in_which_segment_is(char *faultAddr)
{
	char *baseSegmAddr;
	for (int i = 0; i < exec->segments_no; i++)
	{
		baseSegmAddr = (char *)exec->segments[i].vaddr;
		if (faultAddr >= baseSegmAddr && faultAddr <= baseSegmAddr + exec->segments[i].mem_size)
			return i;
	}
	return -1;
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	char *faultAddr, *addrMapped;
	int perm, segmVaddr, segmOffset, fileSize;
	int memSize, pageNr, segNr, fileOffset;
	void *pageAddr, *rc;

	if (signum != SIGSEGV)
	{
		return;
	}

	if (info->si_code == SEGV_ACCERR)
	{
		signal(SIGSEGV, SIG_DFL);
		return;
	}

	faultAddr = (char *)info->si_addr;
	segNr = in_which_segment_is(faultAddr);

	if (segNr == -1)
	{
		signal(SIGSEGV, SIG_DFL);
		return;
	}

	segmVaddr = exec->segments[segNr].vaddr;
	pageNr = ((int)faultAddr - segmVaddr) / pageSize;
	// pageNr = (ALIGN_DOWN((int)faultAddr, pageSize) - segmVaddr) / pageSize;

	perm = exec->segments[segNr].perm;
	memSize = exec->segments[segNr].mem_size;
	segmOffset = exec->segments[segNr].offset;
	fileSize = exec->segments[segNr].file_size;
	fileOffset = pageNr * pageSize + segmOffset;
	pageAddr = (void *)ALIGN_DOWN((int)faultAddr, pageSize);

	if (pageNr * pageSize < fileSize && (pageNr + 1) * pageSize <= fileSize)
	{
		addrMapped = mmap(pageAddr, pageSize, perm, MAP_FIXED | MAP_PRIVATE, fd, fileOffset);
		if (addrMapped == MAP_FAILED)
		{
			perror("mmap error!");
			return;
		}
	}
	else if (pageNr * pageSize < fileSize && (pageNr + 1) * pageSize >= fileSize)
	{
		addrMapped = mmap(pageAddr, pageSize, perm, MAP_FIXED | MAP_PRIVATE, fd, fileOffset);
		if (addrMapped == MAP_FAILED)
		{
			perror("mmap error!");
			return;
		}

		if (fileSize != memSize)
		{
			rc = memset((void *)segmVaddr + fileSize, 0, (pageNr + 1) * pageSize - fileSize);
			if (rc == NULL)
			{
				perror("memset error!");
				return;
			}
		}
	}
	else
	{
		addrMapped = mmap(pageAddr, pageSize, perm, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (addrMapped == MAP_FAILED)
		{
			perror("mmap error!");
			return;
		}
	}
}

int so_init_loader(void)
{
	int rc = 0;
	struct sigaction action;

	pageSize = getpagesize();
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_sigaction = segv_handler;

	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	rc = sigaction(SIGSEGV, &action, NULL);

	if (rc < 0)
	{
		perror("Sigaction error!");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	int rc = 0;
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	fd = open(path, O_RDONLY, 0644);
	if (fd < 0)
	{
		perror("Open error!");
		return -1;
	}

	so_start_exec(exec, argv);

	rc = close(fd);
	if (rc < 0)
	{
		perror("Close error!");
		return -1;
	}

	return -1;
}