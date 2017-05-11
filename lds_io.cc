#include <stdlib.h>
#include <stdio.h>

#include "db/lds_io.h"
#include "util/coding.h" //in LevelDB

#define MAGIC "LDSX"
extern char * OnlineMap; //lds.cc
extern uint64_t SlotTotal;

namespace leveldb {



size_t Slot_write(const void * ptr, size_t size, size_t count, LDS_Slot * slot ){
		//only write to buffer
		uint32_t write_bytes;//payload
		write_bytes=size*count;
		//printf("lds_io.cc, SLot_write, write_bytes=%d\n",write_bytes);
		
		if(slot->size + write_bytes > SLOT_SIZE){
			fprintf(stderr,"lds_io.cc, SLot_write, overflow,exit, name=%s, slot->size=%d\n",slot->file_name.c_str(),slot->size );
			
			exit(9);
		
		}
		memcpy(slot->buffer + slot->write_head,  ptr, write_bytes);
		
		slot->write_head += write_bytes ;
		slot->size += write_bytes;
		//printf("lds_io.cc, SLot_write, size=%d\n",slot->size);

		return write_bytes;

}

size_t Log_write(const void * ptr, size_t size, size_t count, LDS_Log * log ){
	//only write to buffer

	uint32_t write_bytes;//payload
	write_bytes=size*count;
	//printf("lds_io.cc, Log_write, size=%d,data=%s\n", write_bytes,ptr);
	

	uint32_t header_size=20;//magic[4],type[4],sn[8],size[4];
	char head[header_size];
	sprintf(head,"%s",MAGIC);
	EncodeFixed32(head+4, 2);//2 presents the type is common delta version
	EncodeFixed64(head+8, 0x333322);//log->sn
	EncodeFixed64(head+16, write_bytes);//
	
	uint32_t crc=0x77777777;//crc[4], call the crc32c::Extend(type_crc_[t], ptr, n) to calculate the crc
	uint32_t crc_size=sizeof(crc);
	char crc_buf[crc_size];
	EncodeFixed32(crc_buf, crc);
	
	if(log->file_name.find("MANIFEST")!=-1){
		//printf("lds_io.cc, Log_write, log->size=%d\n",  log->size);
		if(log->size +  header_size+write_bytes + crc_size > VERSION_LOG_SIZE){
			fprintf(stderr,"lds_io.cc,  Log_write,version area overflow\n");
			exit(9);
		}
	}
	else if(log->file_name.find(".log")!=-1){
		//printf("lds_io.cc, Log_write, log->size=%d\n",  log->size);
		if(log->size +  header_size+write_bytes + crc_size > BACKUP_SIZE){
			fprintf(stderr,"lds_io.cc, Log_write, backup area overflow\n");
			exit(9);
		}
	}
	
	memcpy(log->buffer + log->write_head,  head, header_size);//copy the header

	memcpy(log->buffer + log->write_head+header_size,  ptr, write_bytes);//copy the payload

	

	memcpy(log->buffer + log->write_head+header_size+write_bytes,  crc_buf, crc_size);//copy the crc



	log->write_head += header_size+write_bytes + crc_size;
	
	log->size += header_size+write_bytes + crc_size;
	

	return write_bytes;
	
}

size_t Slot_flush(LDS_Slot *slot){
		//printf("lds_io.cc, Slot_flush, begin\n");
		
		
		lseek64(slot->fd, slot->phy_offset+ slot->flush_offset, SEEK_SET);
		
		uint64_t flush_bytes = slot->write_head - slot->flush_offset;
		
		write(slot->fd, slot->buffer+ slot->flush_offset, flush_bytes);
		
		slot->flush_offset =  slot->write_head;
		
		//printf("lds_id.cc, Slot_flush, end, name=%s, slot->flush_offset=%d\n",slot->file_name.c_str(),slot->flush_offset);

		//exit(9);
		return flush_bytes;
		
}

size_t Slot_sync(LDS_Slot *slot){
	//printf("lds_io.cc, Slot_sync, begin, chun size=%d\n", slot->size);
	//printf("lds_io.cc, Slot_sync, begin, chun size=%d, flush offset=%d\n", slot->size, slot->flush_offset);
	Slot_flush(slot);


	char coded_size[8];
	EncodeFixed64(coded_size, slot->size);

	lseek64(slot->fd, slot->phy_offset+ (SLOT_SIZE -8), SEEK_SET);//8 bytes for the chunk size	
	write(slot->fd, coded_size, 8);
	
	int res;
	res=sync_file_range(slot->fd, slot->phy_offset, SLOT_SIZE , SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER );//SYNC_FILE_RANGE_WRITE
	//printf("lds_io.cc, Slot_sync, begin, name=%s, phyoffset=%d\n", slot->file_name.c_str(), slot->phy_offset );
	
	if(res!=0){	
		
		fprintf(stderr,"lds_io.cc, Slot_sync, res error, exit\n");
		exit(3);
	}
	return res;
	//sleep(999);

	//exit(1);

}

size_t Slot_close(LDS_Slot *slot){
	delete slot;
}

size_t Log_flush(LDS_Log * log){

	//printf("lds_id.cc, Log_flush, fd=%d\n", log->fd);

	lseek64(log->fd, log->phy_offset+ log->flush_offset, SEEK_SET);
	
	uint64_t flush_bytes = log->write_head - log->flush_offset;
	
	write(log->fd, log->buffer+ log->flush_offset, flush_bytes);
	
	log->flush_offset =  log->write_head;
	
	//printf("lds_id.cc, Log_flush, end\n");

	//exit(9);
	return flush_bytes;
}

size_t Log_close(LDS_Log * log){
	delete log;
}
int decode(char *raw_data, LDS_Log *log){

	uint32_t header_size=20;//magic[4],type[4],sn[8],size[4];
	printf("lds_io.cc, decode, begin\n");
	while(1){
		char magic[4];
		strncpy(magic, raw_data,4);
		int cmp= strncmp(magic, MAGIC,4);
		//printf("lds_io.cc, decode, raw_data=%s,magic=%s, cmp=%d\n",raw_data,magic,cmp);
		if(cmp!=0){
			break;
		}
		uint32_t payload_size= *(uint32_t*)(raw_data+16);
		//printf("lds_io.cc, decode, payload size=%d\n",payload_size);

		raw_data+= 20;
		
		memcpy(log->buffer + log->size, raw_data, payload_size);
		log->size += payload_size;
		raw_data +=payload_size +4;//crc
		
	}

	

}
size_t Log_read(void * ptr, size_t size, size_t count, LDS_Log *log){

		//printf("lds_io.cc, Log_read, begin, read_buf=%p\n",log->read_buf);
		if(log->read_buf==NULL){
			printf("lds_io.cc, Log_read, buff is null, fd=%d, size=%lld\n",log->fd, log->size);
			//here first load the data to memory, decode the valid data and copy them to read_buf.
			char *read_buf=  (char*)mmap(NULL, log->load_size, PROT_READ, MAP_SHARED, log->fd,log->phy_offset);
			//printf("lds_io.cc, Log_read, read_buf=%p,read_buf=%s\n",read_buf, read_buf );
			decode(read_buf, log);

			//printf("lds_io.cc, Log_read, after decode, size=%lld,buffer=%s\n",log->size, log->buffer );
			
			//exit(9);
		}
		//now provide read service, content is stored in log->buffer, bytes are log->size. read position is at log->read_offset.

		size_t request_bytes= size*count;
		size_t remained_bytes = log->size - log->read_offset;

		size_t supply_bytes= remained_bytes > request_bytes? request_bytes : remained_bytes;
		if(supply_bytes>0){
			memcpy(ptr, log->buffer + log->read_offset, supply_bytes);
			log->read_offset += supply_bytes;
		}
		//printf("lds_io.cc, Log_read, end, supply_bytes=%d\n",supply_bytes);
		return supply_bytes;

}



uint64_t Alloc_slot(uint64_t next_file_number_){
	//printf("lds_io.cc, Alloc_slot, begin\n");
	//this function will alloc a free slot number according to the online-map;
	//exit(9);
	
	uint64_t result;
	result = next_file_number_ %SlotTotal;
	
	uint64_t final_number;
	uint64_t temp;
	for(temp=result; temp< SlotTotal; temp++){//to refirm that the slot is free. if not, find the next front it.
		if(OnlineMap[temp]==0){//it is free
			OnlineMap[temp]==1;
			final_number= next_file_number_ + (temp-result);//reverse map
			return final_number;
		}
	
	}

	fprintf(stderr,"lds_io.cc, Alloc_slot, round back\n");
	for(temp= 0 ;temp<result;temp++){ //round back
		if(OnlineMap[temp]==0){
			OnlineMap[temp]==1;
			final_number= next_file_number_ + (SlotTotal- result) + temp;
			
			return final_number;
		}
			
	}
	//if come to there, there is no free slot to alloc
	fprintf(stderr,"lds_io.cc, storage full,exit!\n");
	exit(9);
}


uint64_t read_chunk_size(LDS_Slot *slot){
	uint64_t offset= slot->phy_offset+ (SLOT_SIZE -8);
	
	
	//printf("lds_io.cc, read_chunk_size,slot->phy_offset=%d, offset=%llu\n",slot->phy_offset,offset);
	char coded_size[8];
	
	fsync(slot->fd);
	
	//sleep(1);
	lseek64(slot->fd, offset, SEEK_SET);//8 bytes for the chunk size
	read(slot->fd, coded_size, 8);
	
	
	
	//printf("lds_io.cc, read_chunk_size,coded_size=%s\n",coded_size);

	uint64_t size =*(uint64_t*)coded_size;
	
	return size;
	
}


}//end leveldb