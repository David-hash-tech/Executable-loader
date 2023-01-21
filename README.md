# README

## Function: in_which_segment_is(faultAddr)
- Verifies if the address that generated the SIGSEGV signal is located in a segment of the executable
- For each segment in the executable file, checks if the faultAddr is greater than the start address of the segment and less than the end address of the segment
- If the above condition is satisfied, the function returns the index of the segment in the exec->segments vector

## Function: segv_handler
- Verifies if the received signal is SIGSEGV. If another signal is received, the function ends as it represents a handler for the SIGSEGV signal
- Checks if the SIGSEGV signal is the result of an unauthorized access to a already mapped memory area (the user did not have permission to access that memory area). In this case, the default handler of the SIGSEGV signal is called
- Determines the segment in which the address that generated the SIGSEGV signal is located. If the address is not part of any segment of the file, the default handler is called as the cause of the signal does not come from our executable
- Initializes the necessary variables:
    - segmVaddr -> the address where the segment starts
    - pageNr -> the number of the page in the segment where faultAddr is located
    - perm -> the permissions of the segment
    - memSize -> the size of the segment in memory
    - segmOffset -> the offset in the file where the segment starts
    - fileSize -> how much of the file is in the segment
    - fileOffset -> the offset in the file that remains to be mapped
    - pageAddr -> the address of the page to be loaded (where faultAddr is located)
- If the current page's content is entirely occupied by data from the file, the entire page's content should be mapped in memory
- If only part of the page's content is occupied by data from the file, we map the page in memory and zero out the rest of the page using the memset function with the value 0. We avoid the situation where the page is full and the next page is empty (fileSize != memSize), in which case we would zero out the next page, to which we do not have access
- If none of the above cases are met, we only allocate memory for one page
