#include <stdlib.h>
#include <stdio.h>

#include "db/lds_io.h"

extern int  is_storage_inited;

char * OnlineMap;
uint64_t SlotTotal;

namespace leveldb{


LDS::LDS(const std::string& storage_path){
	int res=Storage_init(storage_path);//

}

LDS::LDS(const std::string& storage_path, int flash_using_exist){
	int res;

	res=Storage_init(storage_path);//
	//if(flash_using_exist==0){
	//	res=Storage_init(storage_path);//
	//}
	//else if(flash_using_exist==1){

	//	res=LDS_recover(storage_path);
	//}
	//else{
	//	printf("error value of flash_using_exist,%d\n",flash_using_exist);
	//	exit(0);
	//}

}

LDS_Log::LDS_Log(std::string name){
		write_head=0;//indicates the current position to append in LDS buffer
		flush_offset= 0;//indicates the current position from which (until ot the write_head) needs to be flush to OS buffer
		sync_offset=0;//indicates the current position from which (until the flush_offset) needs to be flush to the disk
		file_name=name;
		size=0;		
		
		read_buf=NULL;
		read_offset=0;
		



		//this->buffer=(char *)malloc(SEGMENT_BYTES);
		if(name.find("MANIFEST")!=-1){
			posix_memalign(&(this->buffer),512,VERSION_LOG_SIZE);//in order for direct IO.
			phy_offset=0;
			load_size=VERSION_LOG_SIZE;
			
		}
		else if(name.find(".log")!=-1){
			posix_memalign(& (this->buffer) ,512, BACKUP_SIZE);//in order for direct IO.
			phy_offset= VERSION_LOG_SIZE;

			load_size=BACKUP_SIZE;
		}
		//memset(this->buffer,0,ENTRY_BYTES)

}

LDS_Slot::LDS_Slot(std::string name){
		write_head=0;
		flush_offset= 0;
		sync_offset=0;	
		file_name=name;
		size=0;
		
		posix_memalign(&(this->buffer),512,SLOT_SIZE);//in order for direct IO.
		
		std::string short_file_name=name;
		//printf("in tools.c 111 short_file_name=%s\n",short_file_name.c_str());
		if(short_file_name.find("/dev/")!=-1){		
			int found=name.find_last_of("/");
			//printf("%s ,%s \n",long_file_name.c_str(),long_file_name.substr(found+1).c_str());
			 short_file_name=name.substr(found+1);
			
		}
		
		//printf("lds.cc LDS_Slot, short_file_name=%s\n",short_file_name.c_str());
		//exit(9);
		uint64_t number=atoi(short_file_name.c_str());
		
		
		phy_offset = VERSION_LOG_SIZE + BACKUP_SIZE+ number * SLOT_SIZE;

}

LDS_Slot * LDS::alloc_slot(const std::string& chunk_name){

	LDS_Slot *slot=new LDS_Slot(chunk_name);
	
	
	

	slot->fd=this->slot_fd;
	
	return slot;

}

LDS_Log * LDS::alloc_log(const std::string& name){

	LDS_Log *log=new LDS_Log(name);
	//printf("lds.cc, alloc_log, dev_fd=%d\n", this->dev_fd);
	//exit(9);
	if(name.find("MANIFEST")!=-1){
		log->fd=this->version_fd;
	}	
	else if(name.find(".log")!=-1){
		log->fd=this->backup_fd;
	}	
	return log;


}


// LDS_Log * LDS::alloc_version(const std::string& name){

	// // LDS_Log *log=new LDS_Log(name);
	// // log->fd=this->dev_fd;

	
	// // log->size=VERSION_LOG_SIZE;
	// // log->phy_offset=0;
	
	
	// // return log;


// }
// LDS_Log * LDS::alloc_backup(const std::string& backup){


// }


int LDS:: Storage_init(const std::string& storage_path){

	int fd=-1;
	int fd1=-1;
	int fd2=-1;
	int fd3=-1;
	fd=open(storage_path.c_str(),O_RDWR);
	fd1=open(storage_path.c_str(),O_RDWR);
	fd2=open(storage_path.c_str(),O_RDWR);
	fd3=open(storage_path.c_str(),O_RDWR);

	printf("lds.cc, Storage_init, fd=%d,fd2=%d.fd3=%d\n",fd,fd2,fd3);
	//exit(9);

	if(fd<0){
		printf("lds.cc, Storage_init, open error,exit\n");
		exit(0);
	}
	uint64_t blk64;
	
	if(storage_path.find("/dev/")!=-1){//the path is the raw device
		ioctl(fd, BLKGETSIZE64, &blk64);//reture the size in bytes . This result is real
	}
	else{//the path should be a pre-allocated file
		blk64=lseek(fd, 0, SEEK_END)+1;
		lseek(fd, 0, SEEK_SET)+1;	
	}
	
	
	printf("lds.cc, Storage_init,, device size=【%llu GB】\n",blk64/1024/1024/1024);
	
	this->dev_read_only=( char *)mmap(NULL,blk64 ,PROT_READ, MAP_SHARED,fd,0);
	printf("lds.cc, Storage_init, dev_read_only=%p\n",dev_read_only);

	this->version_fd=fd1;
	this->backup_fd=fd2;
	this->slot_fd=fd3;

	this->slot_amount = ( (blk64) - VERSION_LOG_SIZE - BACKUP_SIZE) /SLOT_SIZE ;

	//printf("lds.cc, Storage_init, slot_amount=%d %d %d\n",this->slot_amount,(blk64/1024/1024),( (VERSION_LOG_SIZE + BACKUP_SIZE)/1024/1024/SLOT_SIZE ));

	printf("lds.cc, Storage_init, slot_amount=%llu\n",this->slot_amount);

	SlotTotal= this->slot_amount;
	//exit(9);
	OnlineMap = (char*) malloc(slot_amount);//now we use one byte for a slot, but one bit is enough
	memset(OnlineMap, 0, slot_amount);

	//exit(0);

	
	
}

int LDS:: LDS_recover(const std::string& storage_path){

}


}//leveldb