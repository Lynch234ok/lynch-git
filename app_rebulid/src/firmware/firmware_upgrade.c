
#include "firmware.h"
//#include "sysconf.h"
#include "inifile.h"
#include <sys/reboot.h>
#include "base/ja_process.h"
#include <sys/mount.h>
#include <sys/prctl.h>

#define FIRMWARE_UPGRADE_SPEED (128 * 1024)

typedef struct FwUpgrade
{
	ssize_t upgrade_size;
	ssize_t total_size;

	pthread_t upgrade_tid;
	uint32_t upgrade_trigger;
	char upgrade_soc[32];
	char upgrade_oemNumber[32];
	int error;
	int need_reboot;
	fUPGRADE_ENV_PREPARE upgrade_env_prepare;
}FwUpgrade_t;
static FwUpgrade_t* _fw_upgrade = NULL;

int FIRMWARE_upgrade_get_errno()
{
	return _fw_upgrade->error;
}

int FIRMWARE_upgrade_get_rate()
{
	if(0 == _fw_upgrade->upgrade_size){
		return 0;
	}
	return _fw_upgrade->upgrade_size * 100 / _fw_upgrade->total_size;
}

static int firmware_upgrade_flash(int flash_fd, off_t flash_offset, int fw_fd, off_t fw_offset, ssize_t data_size)
{
	int ret = 0;
	uint8_t buf[FIRMWARE_UPGRADE_SPEED];

	assert(flash_fd > 0 && fw_fd > 0);

	ret = lseek(fw_fd, fw_offset, SEEK_SET);
	assert(fw_offset == ret);
	ret = lseek(flash_fd, flash_offset, SEEK_SET);
	assert(flash_offset == ret);

	do
	{
		ssize_t read_sz = data_size >= FIRMWARE_UPGRADE_SPEED ? FIRMWARE_UPGRADE_SPEED : data_size;
		if(read_sz < FIRMWARE_UPGRADE_SPEED){
			memset(buf, 0, sizeof(buf));
		}
		// read from firmware
		ret = read(fw_fd,  buf, read_sz);
		assert(ret == read_sz);
		// write to flash
		ret = write(flash_fd, buf, sizeof(buf));
		//assert(ret == read_sz);
		//fdatasync(flash_fd);
		fsync(flash_fd);

		data_size -= read_sz;
		_fw_upgrade->upgrade_size += read_sz;

	}while(data_size > 0);

	return 0;
}

static int firmware_upgrade_flash2(int flash_fd, off_t flash_offset, void *memp, off_t fw_offset, ssize_t data_size)
{
        int ret = 0;
        uint8_t buf[FIRMWARE_UPGRADE_SPEED];
        //static int writetotal = 0;

        assert(flash_fd > 0 && memp != NULL);

        //void *mem_data = memp + sizeof(FwHeader_t);
        memp = memp + fw_offset;

        ret = lseek(flash_fd, flash_offset, SEEK_SET);
        assert(flash_offset == ret);

        do
        {
                ssize_t read_sz = data_size >= FIRMWARE_UPGRADE_SPEED ? FIRMWARE_UPGRADE_SPEED : data_size;
                if(read_sz < FIRMWARE_UPGRADE_SPEED){
                        memset(buf, 0, sizeof(buf));
                }
                // read from firmware
                memcpy(buf, memp, read_sz);
                memp = memp +read_sz;
                // write to flash
                ret = write(flash_fd, buf, sizeof(buf));
                fsync(flash_fd);

                //writetotal = writetotal + read_sz;
                //printf("writetotal = %d\n", writetotal);

                data_size -= read_sz;
                _fw_upgrade->upgrade_size += read_sz;

        }while(data_size > 0);

        return 0;
}
static void* firmware_upgrade(void* arg)
{
	printf("%s start\n", __FUNCTION__);
	int i = 0;
	int ret = 0;
	FwHeader_t fw_header;
	int fid = -1;

//	// detach thread
//	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "firmware_upgrade");

	fid = open(FIRMWARE_IMPORT_FILE, O_RDWR);
	assert(fid > 0);
	ret = read(fid, &fw_header, sizeof(fw_header));
	assert(sizeof(fw_header) == ret);

//	FIRMWARE_dump_rom(FIRMWARE_IMPORT_FILE);

	// get total size to upgrade
	_fw_upgrade->total_size = 0;
	_fw_upgrade->upgrade_size = 0;
	for(i = 0; i < fw_header.block_cnt; ++i){
		_fw_upgrade->total_size += fw_header.block[i].data_size;
	}
	printf("upgrade total size = %d\r\n", _fw_upgrade->total_size);

	// upgrade
	for(i = 0; i < fw_header.block_cnt && _fw_upgrade->upgrade_trigger; ++i){
		int flash_fd = open(fw_header.block[i].flash, O_RDWR);
		printf("upgrade %s offset=%d size = %d\r\n", fw_header.block[i].flash, fw_header.block[i].data_offset, fw_header.block[i].data_size);
		if(flash_fd > 0){
			firmware_upgrade_flash(flash_fd, fw_header.block[i].flash_offset, fid, fw_header.block[i].data_offset, fw_header.block[i].data_size);
			close(flash_fd);
			flash_fd = -1;
		}
	}

	close(fid);
	printf("%s end\n", __FUNCTION__);


	sleep(2);
	if(_fw_upgrade->need_reboot){
#if !defined(PC_TOOLS)
		NK_SYSTEM("reboot"); // must reboot
#endif
	}
	
	pthread_exit(NULL);
}

static void* firmware_upgrade2(void* arg)
{
	printf("%s start\n", __FUNCTION__);
	int i = 0;
	int ret = 0;
	FwHeader_t fw_header;
	int fid = -1;

	// detach thread
	void *up_memp = arg;
	memcpy(&fw_header, up_memp, sizeof(fw_header));
	prctl(PR_SET_NAME, "firmware_upgrade2");

	// get total size to upgrade
	_fw_upgrade->total_size = 0;
	_fw_upgrade->upgrade_size = 0;
	for(i = 0; i < fw_header.block_cnt; ++i){
		_fw_upgrade->total_size += fw_header.block[i].data_size;
	}
	printf("upgrade total size = %d\r\n", _fw_upgrade->total_size);
	
	// upgrade
	for(i = 0; i < fw_header.block_cnt && _fw_upgrade->upgrade_trigger; ++i){
		int flash_fd = open(fw_header.block[i].flash, O_RDWR);
		printf("upgrade %s offset=%d size = %d\r\n", fw_header.block[i].flash, fw_header.block[i].data_offset, fw_header.block[i].data_size);
		if(flash_fd > 0){
			firmware_upgrade_flash2(flash_fd, fw_header.block[i].flash_offset, up_memp, fw_header.block[i].data_offset, fw_header.block[i].data_size);
			close(flash_fd);
			flash_fd = -1;
		}
	}

	sleep(2);
	if(_fw_upgrade->need_reboot){
		printf("fw_upgrade complete. rebooting!\n");
#if defined(WIFI)
		GLOBAL_reboot_system();
#else
		reboot(RB_AUTOBOOT);
#endif
		//NK_SYSTEM("reboot"); // must reboot
	}

	pthread_exit(NULL);
}

int FIRMWARE_upgrade_start(int need_reboot, bool downgrade)
{
	printf("%s start\n", __FUNCTION__);
	if(!_fw_upgrade->upgrade_tid){
		int ret = 0;
		_fw_upgrade->error = FIRMWARE_UPGRADE_ERROR_NONE;
		if(FIRMWARE_import_check(_fw_upgrade->upgrade_soc, &_fw_upgrade->error, downgrade)){
			// import file check ok!!
			//do prepare for upgrade env in each soc
			_fw_upgrade->upgrade_trigger = true;
			_fw_upgrade->need_reboot = need_reboot;
			ret = pthread_create(&_fw_upgrade->upgrade_tid, NULL, firmware_upgrade, NULL);
			assert(0 == ret);
			return 0;
		}
		FIRMWARE_import_recover_memery();		
	}
		FIRMWARE_import_recover_memery();
	return -1;
}

int FIRMWARE_upgrade_start2(bool oem_force, bool downgrade, int need_reboot, void* up_memp, int filelen)
{
	printf("%s start\n", __FUNCTION__);
	if(!_fw_upgrade->upgrade_tid){
		int ret = 0;
		_fw_upgrade->error = FIRMWARE_UPGRADE_ERROR_NONE;
		if(FIRMWARE_import_check2(oem_force, downgrade, _fw_upgrade->upgrade_oemNumber, _fw_upgrade->upgrade_soc, &_fw_upgrade->error, up_memp, filelen)){
			// import file check ok!!
			_fw_upgrade->upgrade_trigger = true;
			_fw_upgrade->need_reboot = need_reboot;
			ret = pthread_create(&_fw_upgrade->upgrade_tid, NULL, firmware_upgrade2, up_memp);
			assert(0 == ret);
			return 0;
		}
		FIRMWARE_import_recover_memery	();
	}
		FIRMWARE_import_recover_memery();
	return -1;
}

int FIRMWARE_upgrade_import_check(bool downgrade)
{
	printf("%s start\n", __FUNCTION__);
	if(!_fw_upgrade->upgrade_tid){
		int fw_upgrade_error = FIRMWARE_UPGRADE_ERROR_NONE;
		return FIRMWARE_import_check(_fw_upgrade->upgrade_soc, &fw_upgrade_error, downgrade);
	}
	return false;
}

int FIRMWARE_upgrade_wait()
{
	if(_fw_upgrade->upgrade_tid){
		pthread_join(_fw_upgrade->upgrade_tid, NULL);
		_fw_upgrade->upgrade_trigger = false;
		_fw_upgrade->upgrade_tid = (pthread_t)NULL;
		return 0;
	}
	return -1;
}

int FIRMWARE_upgrade_cancel()
{
	if(_fw_upgrade->upgrade_tid){
		_fw_upgrade->upgrade_trigger = false;
		return FIRMWARE_upgrade_wait();
	}
	return -1;
}

int FIRMWARE_is_upgrading()
{
	if(_fw_upgrade){
		return  _fw_upgrade->upgrade_trigger;
	}
	return 0;
}

int FIRMWARE_upgrade_init(const char *info)
{
	if(!_fw_upgrade){
		_fw_upgrade = calloc(sizeof(FwUpgrade_t), 1);
		lpINI_PARSER inf = NULL;
		_fw_upgrade->upgrade_size = 0;
		_fw_upgrade->total_size = 0;
		_fw_upgrade->upgrade_tid = (pthread_t)NULL;
		_fw_upgrade->upgrade_trigger = false;
		_fw_upgrade->error = FIRMWARE_UPGRADE_ERROR_NONE;
		memset(_fw_upgrade->upgrade_soc, 0, sizeof(_fw_upgrade->upgrade_soc));
		memset(_fw_upgrade->upgrade_oemNumber, 0, sizeof(_fw_upgrade->upgrade_oemNumber));
		inf = OpenIniFile(info);
		if(inf){
			char ini_buf[1024] = {""};
			strncpy(_fw_upgrade->upgrade_soc, 
				inf->read_text(inf, "FIRMWARE", "model", "MODEL", ini_buf, sizeof(ini_buf)), 
				sizeof(_fw_upgrade->upgrade_soc));
			CloseIniFile(inf);
		}
		/*SYSCONF_t *sysconf = SYSCONF_dup();
		if(0 == strlen(_fw_upgrade->upgrade_soc) && sysconf){
			strncpy(_fw_upgrade->upgrade_soc, sysconf->ipcam.info.device_soc, sizeof(_fw_upgrade->upgrade_soc));
		}*/
		printf("upgrade_soc:%s\r\n", _fw_upgrade->upgrade_soc);
		return 0;
	}
	return -1;
}

void FIRMWARE_upgrade_destroy()
{
	if(_fw_upgrade){
		FIRMWARE_upgrade_cancel();

		free(_fw_upgrade);
		_fw_upgrade = NULL;
	}
}

void FIRMWARE_upgrade_env_callback_set(fUPGRADE_ENV_PREPARE fCallback)
{
	if(_fw_upgrade){
		_fw_upgrade->upgrade_env_prepare = fCallback;
	}
}

void* FIRMWARE_upgrade_env_prepare(void *param)
{
	if(_fw_upgrade->upgrade_env_prepare){
	return	_fw_upgrade->upgrade_env_prepare(param);
	}
	sleep(5);
}

int FIRMWARE_upgrade_set_oem_number(char *oemNumber)
{
	if(NULL != oemNumber && strlen(oemNumber) > 0){
		snprintf(_fw_upgrade->upgrade_oemNumber, sizeof(_fw_upgrade->upgrade_oemNumber), "%s", oemNumber);
		printf("<<<set firmware upgrade oemNumber:%s>>>\n", _fw_upgrade->upgrade_oemNumber);
	}
	return 0;
}

int FIRMWARE_upgrade_parse_rom_from_file(char * file_path, char *output_file_path)
{
	printf("%s start\n", __FUNCTION__);
	int i = 0;
	int ret = 0;
	FwHeader_t fw_header;
	int fid = -1;

	if(FIRMWARE_import_check1(_fw_upgrade->upgrade_soc, &_fw_upgrade->error,file_path)){
		fid = open(file_path, O_RDONLY);
		assert(fid > 0);
		ret = read(fid, &fw_header, sizeof(fw_header));
		assert(sizeof(fw_header) == ret);

		// get total size to upgrade
		_fw_upgrade->total_size = 0;
		_fw_upgrade->upgrade_size = 0;
		for(i = 0; i < fw_header.block_cnt; ++i){
			_fw_upgrade->total_size += fw_header.block[i].data_size;
		}
		printf("fw_header.block_cnt:%d\n", fw_header.block_cnt);
		// seperate mtd from rom
		char path[512], block[128], shell_cmd[512];
		memset(path, 0, sizeof(path));
		memset(block, 0, sizeof(block));
		for(i = 0; i < fw_header.block_cnt; ++i){
			int data_size = fw_header.block[i].data_size%1024;
			if(data_size != 0){
				data_size = fw_header.block[i].data_size/1024 + 1;
			}else{
				data_size = fw_header.block[i].data_size/1024;
			}
			snprintf(block, sizeof(block),"%s %d %d ",
				fw_header.block[i].flash,
				fw_header.block[i].data_offset/1024,
				data_size);
			strcat(path, block);
			/*snprintf(path, sizeof(path),"%s%s", output_file_path, fw_header.block[i].flash);
			int flash_fd = open(path, O_RDWR|O_CREAT);
			printf("upgrade %s offset=%d size = %d\r\n", path, fw_header.block[i].data_offset, fw_header.block[i].data_size);
			if(flash_fd > 0){
				firmware_upgrade_flash(flash_fd, fw_header.block[i].flash_offset, fid, fw_header.block[i].data_offset, fw_header.block[i].data_size);
				close(flash_fd);
				flash_fd = -1;
			}*/
		}
		umount2("/media/custom", MNT_DETACH);
		snprintf(shell_cmd, sizeof(shell_cmd), "/usr/share/ipcam/shell/upgrade.sh %s %s &", file_path, path);	
		printf("%s:%s\n", __FUNCTION__, shell_cmd);
#if !defined(PC_TOOLS)
		NK_SYSTEM(shell_cmd);
#endif
        close(fid);
		return 0;
	}
	
	return -1;
}

int FIRMWARE_upgrade_parse_rom_from_memery(void *memsrc, char *output_file_path)
{
	printf("%s start\n", __FUNCTION__);
	int i = 0;
	int ret = 0;
	FwHeader_t fw_header;
	int fid = -1;

	// detach thread
	void *up_memp = memsrc;
	memcpy(&fw_header, up_memp, sizeof(fw_header));

	// get total size to upgrade
	_fw_upgrade->total_size = 0;
	_fw_upgrade->upgrade_size = 0;
	for(i = 0; i < fw_header.block_cnt; ++i){
		_fw_upgrade->total_size += fw_header.block[i].data_size;
	}
	printf("upgrade total size = %d\r\n", _fw_upgrade->total_size);

	// upgrade
	char path[512], block[128], shell_cmd[512];
	memset(path, 0, sizeof(path));
	memset(block, 0, sizeof(block));
	for(i = 0; i < fw_header.block_cnt && _fw_upgrade->upgrade_trigger; ++i){
		int data_size = fw_header.block[i].data_size%1024;
		if(data_size != 0){
			data_size = fw_header.block[i].data_size/1024 + 1;
		}else{
			data_size = fw_header.block[i].data_size/1024;
		}
		snprintf(block, sizeof(block),"%s %d %d ",
			fw_header.block[i].flash,
			fw_header.block[i].data_offset/1024,
			data_size);
		strcat(path, block);
	}
	umount2("/media/custom", MNT_DETACH);
	snprintf(shell_cmd, sizeof(shell_cmd), "/tmp/upgrade.sh %s %s &", "/tmp/firmware/firmware.rom", path);	
	printf("%s:%s\n", __FUNCTION__, shell_cmd);
#if !defined(PC_TOOLS)
	NK_SYSTEM(shell_cmd);
#endif
	return -1;
}
