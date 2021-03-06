	#include "mem.h"
	#include "stdlib.h"
	#include "string.h"
	#include <pthread.h>
	#include <stdio.h>

	static BYTE _ram[RAM_SIZE];

	static struct
	{
		uint32_t proc; // ID of process currently uses this page
		int index;	   // Index of the page in the list of pages allocated
					// to the process.
		int next;	   // The next page in the list. -1 if it is the last
					// page.
	} _mem_stat[NUM_PAGES];

	static pthread_mutex_t mem_lock;

	void init_mem(void)
	{
		memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
		memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
		pthread_mutex_init(&mem_lock, NULL);
	}

	/* get offset of the virtual address */
	static addr_t get_offset(addr_t addr)
	{
		return addr & ~((~0U) << OFFSET_LEN);
	}

	/* get the first layer index */
	static addr_t get_first_lv(addr_t addr)
	{
		return addr >> (OFFSET_LEN + PAGE_LEN);
	}

	/* get the second layer index */
	static addr_t get_second_lv(addr_t addr)
	{
		return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
	}

	/* Search for page table table from the a segment table */
	static struct page_table_t *get_page_table(
		addr_t index, // Segment level index
		struct seg_table_t *seg_table)
	{ 
		return seg_table->table[index].pages;
	}

	/* Translate virtual address to physical address. If [virtual_addr] is valid,
	* return 1 and write its physical counterpart to [physical_addr].
	* Otherwise, return 0 */
	static int translate(
		addr_t virtual_addr,   // Given virtual address
		addr_t *physical_addr, // Physical address to be returned
		struct pcb_t *proc)
	{ // Process uses given virtual address

		/* Offset of the virtual address */
		addr_t offset = get_offset(virtual_addr);
		/* The first layer index */
		addr_t first_lv = get_first_lv(virtual_addr);
		/* The second layer index */
		addr_t second_lv = get_second_lv(virtual_addr);

		/* Search in the first level */
		struct page_table_t *page_table = NULL;
		page_table = get_page_table(first_lv, proc->seg_table);
		if (page_table == NULL)
		{
			return 0;
		}

		if (page_table->table[second_lv].v_index == -1){
			return 0;
		}

		else
		{
			*physical_addr = (page_table->table[second_lv].p_index << OFFSET_LEN) + offset;
			return 1;
		}
	}

	addr_t alloc_mem(uint32_t size, struct pcb_t *proc)
	{
		pthread_mutex_lock(&mem_lock);
		addr_t ret_mem = 0;
		/* TODO: Allocate [size] byte in the memory for the
		* process [proc] and save the address of the first
		* byte in the allocated memory region to [ret_mem].
		* */

		uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1 : size / PAGE_SIZE; // Number of pages we will use
		int mem_avail = 0;																   // We could allocate new memory region or not?

		/* First we must check if the amount of free memory in
		* virtual address space and physical address space is
		* large enough to represent the amount of required 
		* memory. If so, set 1 to [mem_avail].
		* Hint: check [proc] bit in each page of _mem_stat
		* to know whether this page has been used by a process.
		* For virtual memory space, check bp (break pointer).
		* */
		int free_pages = 0;
		for (int i = 0; i < NUM_PAGES; i++)
		{
			if (_mem_stat[i].proc == 0)
				free_pages++;
		}
		mem_avail = free_pages > num_pages ? 1 : 0;
		if (proc->bp + num_pages * PAGE_SIZE > (1 << ADDRESS_SIZE))
			mem_avail = 0;
		if (mem_avail)
		{
			/* We could allocate new memory region to the process */
			ret_mem = proc->bp;
			proc->bp += num_pages * PAGE_SIZE;
			/* Update status of physical pages which will be allocated
			* to [proc] in _mem_stat. Tasks to do:
			* 	- Update [proc], [index], and [next] field
			* 	- Add entries to segment table page tables of [proc]
			* 	  to ensure accesses to allocated memory slot is
			* 	  valid. */
			int page_index = 0;
			int pre_index = -1;
			addr_t temp_ret = ret_mem;
			struct page_table_t *page_table = NULL;
			for (int i = 0; i < num_pages; i++)
			{
				while (_mem_stat[page_index].proc != 0)
				{
					page_index++;
				}
				_mem_stat[page_index].proc = proc->pid;
				_mem_stat[page_index].index = i;
				if (pre_index != -1)
					_mem_stat[pre_index].next = page_index * PAGE_SIZE >> OFFSET_LEN;
				pre_index = page_index;
				page_table = get_page_table(get_first_lv(temp_ret), proc->seg_table);
				if (page_table == NULL)
				{
		
					proc->seg_table->table[get_first_lv(temp_ret)].pages = malloc(sizeof(struct page_table_t));
					page_table = proc->seg_table->table[get_first_lv(temp_ret)].pages;
					proc->seg_table->table[get_first_lv(temp_ret)].v_index = get_first_lv(temp_ret);
					page_table->size = 1 << PAGE_LEN;
					
					for (int i = 0 ; i < page_table->size ; ++i){
						page_table->table[i].v_index = -1;
						page_table->table[i].p_index = -1;
					}
					
				}
				page_table->table[get_second_lv(temp_ret)].v_index = get_second_lv(temp_ret);
				page_table->table[get_second_lv(temp_ret)].p_index = get_second_lv(page_index * PAGE_SIZE);

				// page_table->size++;
				temp_ret += PAGE_SIZE;
			}
			_mem_stat[page_index].next = -1;
		}
		pthread_mutex_unlock(&mem_lock);
		return ret_mem;
	}

	int free_mem(addr_t address, struct pcb_t *proc)
	{
		/*TODO: Release memory region allocated by [proc]. The first byte of
		* this region is indicated by [address]. Task to do:
		* 	- Set flag [proc] of physical page use by the memory block
		* 	  back to zero to indicate that it is free.
		* 	- Remove unused entries in segment table and page tables of
		* 	  the process [proc].
		* 	- Remember to use lock to protect the memory from other
		* 	  processes.  */
		pthread_mutex_lock(&mem_lock);
		int next = 0;
		addr_t *p_addr = malloc(sizeof(addr_t));
		if (translate(address, p_addr, proc) == 0)
			return 0;
		while (next != -1)
		{
			next = _mem_stat[(*p_addr) >> OFFSET_LEN].next;
			_mem_stat[(*p_addr) >> OFFSET_LEN].proc = 0;
			struct page_table_t * page_table = get_page_table(get_first_lv(address), proc->seg_table);
			if (page_table){
				page_table->table[get_second_lv(address)].p_index = -1;
				page_table->table[get_second_lv(address)].v_index = -1;
			}
			address += PAGE_SIZE;
			translate(address, p_addr, proc);
		}
		pthread_mutex_unlock(&mem_lock);
		return 0;
	}

	int read_mem(addr_t address, struct pcb_t *proc, BYTE *data)
	{
		addr_t physical_addr;
		if (translate(address, &physical_addr, proc))
		{
			*data = _ram[physical_addr];
			return 0;
		}
		else
		{
			return 1;
		}
	}

	int write_mem(addr_t address, struct pcb_t *proc, BYTE data)
	{
		addr_t physical_addr;
		if (translate(address, &physical_addr, proc))
		{
			_ram[physical_addr] = data;
			return 0;
		}
		else
		{
			return 1;
		}
	}

	void dump(void)
	{
		int i;
		for (i = 0; i < NUM_PAGES; i++)
		{
			if (_mem_stat[i].proc != 0)
			{
				printf("%03d: ", i);
				printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
					i << OFFSET_LEN,
					((i + 1) << OFFSET_LEN) - 1,
					_mem_stat[i].proc,
					_mem_stat[i].index,
					_mem_stat[i].next);
				int j;
				for (j = i << OFFSET_LEN;
					j < ((i + 1) << OFFSET_LEN) - 1;
					j++)
				{

					if (_ram[j] != 0)
					{
						printf("\t%05x: %02x\n", j, _ram[j]);
					}
				}
			}
		}
	}