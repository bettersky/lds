#include <stdlib.h>
#include <stdio.h>

#include "db/lds.h"


namespace leveldb{



size_t Slot_write(const void * ptr, size_t size, size_t count, LDS_Slot * slot );

size_t Slot_flush(LDS_Slot *slot);

size_t Slot_sync(LDS_Slot *slot);

size_t Slot_close(LDS_Slot *slot);

size_t Slot_read(void * ptr, size_t size, size_t count, LDS_Slot *slot);

size_t Log_write(const void * ptr, size_t size, size_t count, LDS_Log * log );//package the fresh log buffer.

size_t Log_flush(LDS_Log * log);//flush the buffer to OS buffer.

size_t Log_sync(LDS_Log * log);//sync the packages.

size_t Log_close(LDS_Log * log);//flush the buffer to OS buffer.

size_t Log_read(void * ptr, size_t size, size_t count, LDS_Log *log);

uint64_t Alloc_slot(uint64_t next_file_number_);

uint64_t read_chunk_size(LDS_Slot *slot);


class LogObject{
	public:

		char magic[4];
		uint8_t type;
		uint64_t sn;
		uint32_t size;
		void *payload;
		uint32_t crc;

		
		int package(const void * ptr, size_t size, size_t count){


		}

		
};




}//leveldb