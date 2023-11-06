#include "shm.h"
#include "log.h"
#include "misc.h"
#include "plt.h"

#define shmgetcmd() (pshm->cmd)
#define ack() pshm->rsp = 0
#define nack() pshm->rsp = -1

static sem_t *SEMSHMPING;
static sem_t *SEMSHMPONG;
static struct shm_t *pshm = NULL;
static const char *pstr_sem_name_ping = "sta_sem_ping";
static const char *pstr_sem_name_pong = "sta_sem_pong";

static void *shm_thrd_main(void *thd_param)
{
	struct shmparam_t *param = &(pshm->param);
	unsigned char *shmbuf = pshm->buf;
	int idx;

	log_dbg("%s, ++", __func__);

	while (1)
	{
		// wait for cmd
		sem_wait(SEMSHMPING);
		while (sem_trywait(SEMSHMPING) == 0)
			;
		// log_dbg("%s, get cmd:%d", __func__, shmgetcmd());
		switch (shmgetcmd())
		{
		case CMD_STA_FETCH:
			misc_gen_datetimestr(mdl.sztime, sizeof(mdl.sztime));
			memcpy((void *)shmbuf, (void *)(&mdl), sizeof(struct mdl_t));
			ack();
			break;

		case CMD_STA_SENDCMD:
			pshm->rsp = sta_send_cmd(param->val);
			break;

		case CMD_STA_SET_ACTIVEPSET:
			pshm->rsp = sta_set_aps(param->val);
			break;

		case CMD_CHAN_SET_DBG:
			pshm->rsp = chan_set_dbg(param->idx, param->val);
			break;

		case CMD_CHAN_SET_EN:
			pshm->rsp = chan_set_en(param->idx, param->val);
			break;

		case CMD_CHAN_RESET:
			pshm->rsp = chan_reset(param->idx);
			break;

		default:
			log_dbg("unknown cmd:%d\n", shmgetcmd());
			break;
		}

		// log_dbg("%s, cmd:%d done, sem post", __func__, shmgetcmd());
		sem_post(SEMSHMPONG);
	}
	log_dbg("%s, --", __func__);
}

int shm_init(void)
{
	int ret = 0;
	pthread_t thrd;
	int shm_id = -1;
	void *shared_memory = NULL;
	pthread_mutexattr_t attr;

	log_dbg("%s, ++,", __func__);

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	shm_id = shmget((key_t)SHMID, sizeof(struct shm_t), 0666 | IPC_CREAT);
	if (shm_id < 0)
	{
		switch (errno)
		{
		case EINVAL:
			log_dbg("%s shmget fail:EINVAL\n", __func__);
			break;

		case EEXIST:
			log_dbg("%s shmget fail:EEXIST\n", __func__);
			break;

		case EIDRM:
			log_dbg("%s shmget fail:EIDRM\n", __func__);
			break;

		case ENOSPC:
			log_dbg("%s shmget fail:ENOSPC\n", __func__);
			break;

		case ENOENT:
			log_dbg("%s shmget fail:ENOENT\n", __func__);
			break;

		case EACCES:
			log_dbg("%s shmget fail:EACCES\n", __func__);
			break;

		case ENOMEM:
			log_dbg("%s shmget fail:ENOMEM\n", __func__);
			break;

		default:
			log_dbg("%s shmget fail:unknown err\n", __func__);
			break;
		}

		ret = -1;
		goto leave;
	}

	shared_memory = shmat(shm_id, NULL, 0);
	if (shared_memory == NULL)
	{
		ret = -2;
		goto leave;
	}

	pshm = (struct shm_t *)shared_memory;
	memset(pshm, 0, sizeof(struct shm_t));
	pthread_mutex_init(&pshm->mutex, &attr);

	SEMSHMPING = sem_open(pstr_sem_name_ping, O_CREAT | O_EXCL, 0644, 0);
	if (SEMSHMPING == SEM_FAILED)
	{
		switch (errno)
		{
		case EEXIST:
			log_dbg("%s, sem_open semshmping fail,EEXIST, try to open it", __func__);
			SEMSHMPING = sem_open(pstr_sem_name_ping, 0, 0644, 0);
			if (SEMSHMPING == SEM_FAILED)
			{
				ret = -3;
				goto leave;
			}
			break;

		default:
			log_dbg("%s, sem_open semshmping fail,unknown error", __func__);
			ret = -4;
			goto leave;
			break;
		}
	}
	SEMSHMPONG = sem_open(pstr_sem_name_pong, O_CREAT | O_EXCL, 0644, 0);
	if (SEMSHMPONG == SEM_FAILED)
	{
		switch (errno)
		{
		case EEXIST:
			log_dbg("%s, sem_open semshmpong fail,EEXIST, try to open it", __func__);
			SEMSHMPONG = sem_open(pstr_sem_name_pong, 0, 0644, 0);
			if (SEMSHMPONG == SEM_FAILED)
			{
				ret = -5;
				goto leave;
			}
			break;

		default:
			log_dbg("%s, sem_open semshmpong fail,unknown error", __func__);
			ret = -6;
			goto leave;
			break;
		}
	}

	if (pthread_create(&thrd, NULL, shm_thrd_main, NULL) != 0)
	{
		log_dbg("%s, pthread_create fail", __func__);
		ret = -10;
		goto leave;
	}

	// success, print something
	// ...

leave:
	log_dbg("%s, --, ret:%d", __func__, ret);
	return ret;
}
