#ifndef P2PSDK_H
#define P2PSDK_H

#ifdef WIN32
#include <Winsock2.h>
typedef  unsigned __int64 JaUInt64;
typedef long int ssize_t;
#else
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
typedef  uint64_t JaUInt64;
#endif

#ifdef __cplusplus
extern "C" {
#endif
    

/**
 *  设备信息
 */
struct DeviceInfo{
    char sn[32];        /* 设备序列号 */
    char version[32];   /* 设备版本号 */
    char vendor[32];    /*  设备厂商 */
    int max_ch;         /* 设备通道数 */
    char **cam_des;  /* 调用P2PSDKSetCamDes 设置, 需要从应用开发组获取摄像头描述字典, 不设置SDK内部默认填充"CAM"*/
    /* 由于localtime调用,不同系统实现存在差异,手动加入时间的校正
     * 当前时区与GMT时间秒的偏移(且需要考虑到夏令时的影响)
     * eg:GMT+800 --> 8*60*60
     * 参考:struct tm {..long tm_gmtoff...};
     * 注意:
     *      1,如果不用此偏移値,记得设置为零
     *      2,更新时区信息,需要调用P2PSDKUpdateDevinfo更新这个偏移值
     * */
    long tm_gmtoff;
};


/**
 *  设备收到连接请求类型，目前只有穿透请求
 *  ip/port 都是网络字节序
 *  用做统计
 */
enum P2P_CONNECTREQ_TYPE {
    P2P_CONNECTREQ_BUBBLE = 0,
    P2P_CONNECTREQ_HOLE = 1,
    P2P_CONNECTREQ_TURN = 2,
};
/**
 *  穿透请求结果
 */
enum P2P_CONNECTREQ_HOLE_ACTION {
    P2P_CONNECTREQ_HOLE_REQ     = 10,
    P2P_CONNECTREQ_HOLE_PUNCH   = 11,
    P2P_CONNECTREQ_HOLE_CONNECT = 12,
};


/**
 *  P2PFrameHead 头类型
 */
enum P2P_FRAMEHEAD_HEADTYPE {
    P2P_FRAMEHEAD_HEADTYPE_LIVE,
    P2P_FRAMEHEAD_HEADTYPE_REPLAY,
    P2P_RRAMEHEAD_HEADTYPE_VOP2P,
    P2P_FRAMEHEAD_HEADTYPE_ALARMMSG,
};
/**
 *  P2P 码流帧类型,PFRAME与OOB帧之间的枚举值扩展用
 */
enum P2P_FRAMEHEAD_FREAMETYPE {
    P2P_FRAMEHEAD_FREAMETYPE_AUDIO, /**<  音频帧 */
    P2P_FRAMEHEAD_FRAMETYPE_AUDIO = 0,  /**<  音频帧 duplicate*/
    P2P_FRAMEHEAD_FRAMETYPE_IFRAME, /**<  视频I帧 */
    P2P_FRAMEHEAD_FRAMETYPE_PFRAME, /**<  视频P帧 */
    P2P_FRAMEHEAD_FRAMETYPE_OOB = 15, /**<  带外数据帧 */
};

/**
 * 回放录像类型
 */
enum P2P_REC_TYPE {
    P2P_REC_TYPE_TIME = (1<<0),   //定时录像
    P2P_REC_TYPE_MOTION = (1<<1), //移动侦测录像
    P2P_REC_TYPE_ALARM = (1<<2),  //报警录像
    P2P_REC_TYPE_MANU = (1<<3),   //手动录像
    P2P_REC_TYPE_ALL = (P2P_REC_TYPE_TIME | P2P_REC_TYPE_MOTION | P2P_REC_TYPE_ALARM | P2P_REC_TYPE_MANU),
};

/**
 * 回放录像质量
 */
enum P2P_REC_QUALITY {
    P2P_REC_QUALITY_LOW,  //低质量
    P2P_REC_QUALITY_HIGN, //高质量
};

/**
 *  录像记录, 用于回放搜索录像列表
 */
struct  rec_record{
    int chn;            /* 录像通道号，从0算起 */
    int type;           /* 录像类型 */
    time_t startTime;   /* 录像开始时间，GMT*/
    time_t endTime;     /* 录像结束时间, GMT*/
    int quality;        /* 录像质量 */
};

typedef struct rec_list{
    int recordIdx;                /* 此次请求的记录索引 */
    int maxRecord;               /* 最大可填充录像记录数 */
    int recordCnt;                /* 实际匹配的当你记录数 */
    int recordTotal;              /* 满足要求的总记录数*/
    struct rec_record *pRecord;   /* 记录填充缓存 */
}RecList;



/******************************************************************************
 *                      P2P FrameHead
 ******************************************************************************/

#define P2P_FRAMEHEAD_MAGIC (0x50325000)
#define P2P_FRAMEHEAD_VERSION (0x01000000)
struct P2PFrameHead{
    unsigned int magic;         /* magic number 固定为 0x50325000 , "P2P\0"*/
    unsigned int version;       /* 版本信息，当前版本为1.0.0.0，固定为0x01000000 */
    unsigned int headtype;      /* 头类型， 0：直播, 1:录像 */
    unsigned int frametype;     /* 帧类型，0：音频帧，1：视频I帧，2：视频P帧 */
    unsigned int framesize;     /* 帧的裸数据长度 /Byte */
    /*
     * 视频帧时间戳 精度ms，
     * 比如:ts_ms/1000 要同步到视频画面显示时间 2016/6/1 12:0:0
     * 注意: 无时区概念,最有效的办法就是统一当作零时区处理,调用gmtime就可以获取到画面对应时间.
     *
     * 方案1:初始化时 指定 DeviceInfo 中tm_gmtoff的値
     * 假设设备处在GMT+800
     * SDK内部结合当前UTC秒(1464753600) 与 tm_gmtoff(28800) 校正到 零时区的 2016/6/1 12:0:0 (1464782400)
     * 也就是说这个ts_ms 给个相对递增的时间就成了
     *
     * 方案2:初始化时 指定 DeviceInfo 中tm_gmtoff的値为零,
     * 直接使用ts_ms, 也就是说必须开发者事先做好校正,传给SDK
     *
     * 建议: 使用方案一
     */
    JaUInt64 ts_ms;
    union {
        /* 视频编码（"H264"，"H265"...）, 帧率，宽、高 */
        struct{
            char enc[8];
            unsigned int fps;
            unsigned int width;
            unsigned int height;
        }v;

        /* 音频编码（"G711A"...），采样率，采样位宽，采样通道数，压缩率 */
        struct{
            char enc[8];
            unsigned int samplerate;
            unsigned int samplewidth;
            unsigned int channels; /* 默认 1 */
            float compress; /* 压缩率 g711 == 2.0 */
        }a;
    };
    /* 录像码流附加属性 */
    unsigned int recchn;  /* 录像码流对应的通道 */
    unsigned int rectype; /* 录像码流的录像类型 */
    unsigned int recquality; /* 录像码流质量 */
};


/******************************************************************************
 *                      P2PSDK FrameHeadV2
 * 调整 live/vop2p/replay/alarmmsg 的属性头放到一个联合体中
 * 以后再加,只需扩充此联合体的类型
 ******************************************************************************/

/**
 * 帧属性头: 将用于直播及语音对讲业务
 */
struct P2PLiveAttr{
    /*
     * 帧类型，0：音频帧，1：视频I帧，2：视频P帧
     */
    unsigned int frametype;
    union {
        /**
         * 视频编码（"H264"，"H265"...）, 帧率，宽、高
         */
        struct{
            char enc[8];
            unsigned int fps;
            unsigned int width;
            unsigned int height;
        }v;
        /**
         * 音频编码（"G711A"...），采样率，采样位宽，采样通道数，压缩率
         *  channels 默认 1, 单声道
         *  compresss 压缩率, G711 == 2.0
         */
        struct{
            char enc[8];
            unsigned int samplerate;
            unsigned int samplewidth;
            unsigned int channels;
            float compress;
        }a;
    };
};



/**
 * 回放帧属性头: 用于回放业务
 */
struct P2PReplayAttr {
    unsigned int frametype;
    union {
        struct{
            char enc[8];
            unsigned int fps;
            unsigned int width;
            unsigned int height;
        }v;
        struct{
            char enc[8];
            unsigned int samplerate;
            unsigned int samplewidth;
            unsigned int channels;
            float compress_ratio;
        }a;
    };
    /*
     * 录像码流附加属性
     */
    int chn;  //录像码流对应的通道
    int type; //录像码流的录像类型
    int quality; //录像码流质量
};

/**
 *  报警信息类型
 */
enum P2P_ALARMMSG_TYPE {
    P2P_ALARMMSG_TYPE_TEXT = 0,
    P2P_ALARMMSG_TYPE_AUDIO = 1,
    P2P_ALARMMSG_TYPE_SMALL_IMG = 2,
    P2P_ALARMMSG_TYPE_IMG = 3,
    P2P_ALARMMSG_TYPE_LARGE_IMG = 4,
    P2P_ALARMMSG_TYPE_VIDEO = 5,
};

/**
 * 报警信息属性头, 用于报警业务
 */
struct P2PAlarmMsgAttr{
    int type;
    int chn;
    union {
        struct {
            char src[64];
            char enc[8];
            unsigned int fps;
            unsigned int width;
            unsigned int height;
        }i;
        struct {
            char src[64];
            char enc[8];
            unsigned int fps;
            unsigned int width;
            unsigned int height;
        }v;
        struct {
            char src[64];
            char enc[8];
            unsigned int samplerate;
            unsigned int samplewidth;
            unsigned int channles;
            float compress_ratio;
        }a;
    };
};


#define P2P_FRAMEHEAD_VERSION_V2 (0x02000000)

struct P2PFrameHeadV2{
    unsigned int magic;         /* magic number 固定为 0x50325000 , "P2P\0"*/
    unsigned int version;       /* 版本信息，当前版本为2.0.0.0，固定为0x02000000 */
    unsigned int headtype;      /* 头类型， 0：直播, 1:录像 */
    unsigned int framesize;     /* 帧的裸数据长度 /Byte */
    /*
     * 视频帧时间戳 精度ms，
     * 比如:ts_ms/1000 要同步到视频画面显示时间 2016/6/1 12:0:0
     * 注意: 无时区概念,最有效的办法就是统一当作零时区处理,调用gmtime就可以获取到画面对应时间.
     *
     * 方案1:初始化时 指定 DeviceInfo 中tm_gmtoff的値
     * 假设设备处在GMT+800
     * SDK内部结合当前UTC秒(1464753600) 与 tm_gmtoff(28800) 校正到 零时区的 2016/6/1 12:0:0 (1464782400)
     * 也就是说这个ts_ms 给个相对递增的时间就成了
     *
     * 方案2:初始化时 指定 DeviceInfo 中tm_gmtoff的値为零,
     * 直接使用ts_ms, 也就是说必须开发者事先做好校正,传给SDK
     *
     * 建议: 使用方案一
     */
    JaUInt64 ts_ms;
    union {
        struct P2PLiveAttr live;
        struct P2PReplayAttr replay;
        struct P2PLiveAttr vop2p;
        struct P2PAlarmMsgAttr msg;
    };
};



/**
 *
 *  P2P设备抽象,用做SDK初始化
 *  包含：
 *   +设备相关信息（设备信息也可以用接口: P2PSDKUpdateDevinfo 单独更新）
 *   +用户上下文
 *   +SDK事件回调函数地址
 */

struct P2PSDKDevice {
 
    struct DeviceInfo info;
    /**
     *  用户上下文
     */
    void *ctx;
    /**
     *  用户校验事件
     *  @param  ctx:    用户上下文
     *  @param  usr:    用户名
     *  @param  pwd:    用户密码
     *  @return 0:      校验成功
     *          -1:     校验失败
     */
    int (*OnAuth)(void *ctx,const char *usr, const char *pwd);
    /**
     *  云台控制事件
     *  控制动作			描述    			参数1					参数2
     *  "stop"          停止云台运动		无						无
     *  "up"			控制云台向上运动	运动速度，取值0~7			无
     *  "down"			控制云台向下运动	运动速度，取值0~7			无
     *  "left"			控制云台向左运动	运动速度，取值0~7			无
     *  "right"         控制云台向右运动	运动速度，取值0~7			无
     *  "auto"          自动水平旋转		开启或停止自动，取值1或者0	无
     *  "iris_o"        控制光圈打开		镜头速度，取值0~7			无
     *  "iris_c"        控制光圈关闭		镜头速度，取值0~7			无
     *  "zoom_i"		调整远焦			镜头速度，取值0~7			无
     *  "zoom_o"        调整近焦			镜头速度，取值0~7			无
     *  "focus_f"		调整变倍大		镜头速度，取值0~7			无
     *  "focus_n"		调整变倍小		镜头速度，取值0~7			无
     *  "aux"			辅助开关1		    开关号0/1				打开或关闭，取值1或0
     *  "preset_s"      设置预置点位置    预置点                   无
     *  "preset_g"      移动到预置点      预置点                   无
     *  "preset_c"      清除预置点位置    预置点                   无
     */
    int (*OnPtzCtrl)(void *ctx, char *chn, char *act, char *param1, char *param2);
    /**
     *  读取码流信息,参考p2pdevice.cpp
     */
    int (*OnSettingsReadStreamInfo)(void *ctx, char *vinNo, char *streamInfo, int streamInfoSz);
    
    /**********************SDK 预览码流操作回调*********************************/
    /**
     *  打开码流直播码流事件
     *  @param  ctx:    sdk上下文
     *  @param  chNo:   码流通道号
     *  @param  streamNo:   码流号（0，1，2---主码流，子码流1，子码流2...)
     *  @return stream_ctx: 直播码流上下文，用做关闭，读取直播码流的动作
     */
    void* (*OnAttachStream)(void *ctx, int chNo, int streamNo);
    /**
     *  关闭直播码流事件
     *  @param  stream_ctx: 直播码流上下文
     */
    int (*OnDetachStream)(void* stream_ctx);
    /**
     *  读取直播视频帧事件,深拷贝
     *  @param stream_ctx:  直播码流上下文
     *  pFrameHead:码流属性头
     *  raw_frame:存放裸帧
     *  raw_frame_max_sz:裸帧最大缓存大小
     *  返回读取到的字节给SDK, 没有数据就返回0, 其他错误返回-1
     */
    ssize_t (*OnReadFrame)(void *stream_ctx, struct P2PFrameHead* pFrameHead, void* pRawFrame, size_t maxRawFrameSz);
    /**
     * @breif
     *  读取直播视频帧事件,引用
     * @param[in] stream_ctx:
     *  直播码流上下文
     * @param[in/out] ppRawFrame
     *  存放裸帧数据的地址
     * @return
     *  返回读取到的字节给SDK, 没有数据就返回0, 其他错误返回-1
     */
    ssize_t (*OnReadFrameEx)(void *stream_ctx, void **ppRawFrame);
    /**
     * @brief
     *  读取视频帧后处理
     * @param[in] stream_ctx
     *  直播码流上下文
     * @param[in] pRawFrame
     *  视频帧数据地址
     */
    void (*OnAfterReadFrame)(void *stream_ctx, void *pRawFrame);
    /**********************SDK 普通事件钩子*********************************/
    /**
     *  设备上线事件
     *  @param  ctx:    用户上下文
     *  @param  eseeid: 上线设备的易视网ID，可在易视网，易视云等客户端直接添加使用
     */
    int (*OnDevOnline)(void *ctx, const char *eseeid);

    /**
     *  设备下线事件
     *  @param  ctx:    用户上下文
     *  @param  eseeid: 离线设备的易视网ID
     */
    int (*OnDevOffline)(void *ctx, const char *eseeid);

    int (*OnDevConnectReq)(void *ctx, int type, int action, unsigned int ip, unsigned short port, unsigned int random);
    
    /*****************************SDK 回放回调接口****************************/
    /**
     *  获取录像文件列表事件
     *  @param  ctx:        用户上下文
     *  @param  chnCnt:     待搜索的录像通道总数，
     *  @param  chn[]:      通道集合，
     *                      eg: 想搜索 ch0 ch1 ch3 的录像， 此时 chnCnt = 3, chn[0]=0, chn[1]=1, chn[2]=3;
     *  @param  startTime： 录像开始点，GMT时间（零时区）
     *  @param  endTime:    录像结束点，GMT时间（零时区）
     *  @param  quality:    录像清晰度，用做多个码流扩展
     *  @param  pList:      录像记录缓存
     */
    int (*OnFetchRecList)(void *ctx, int chnCnt, char chn[], int type, time_t startTime, time_t endTime, int quality, RecList *pLists);
    /**
     *  打开录像文件事件
     *  @param  ctx:        用户上下文
     *  @param  ctx:        用户上下文
     *  @param  chnCnt:     同FetchRecList
     *  @param  chn[]:      同FetchRecList
     *  @param  type:       录像类型
     *  @param  startTime:  录像开始时间，GMT时间（零时区）
     *  @param  endTime:    录像结束时间，GMT时间（零时区）
     *  @param  quality:    录像文件的清晰度，用做多个码流扩展
     *  @return recCtx:     返回打开的录像文件上下文，用做接下来的读取，关闭动作
     */
    void* (*OnOpenRecFile)(void *ctx, int chnCnt, char chn[], int type, time_t startTime, time_t endTime, int quality);
    /**
     *  打开录像文件事件
     *  @param  ctx:        用户上下文
     *  @param  ctx:        用户上下文
     *  @param  chnCnt:     同FetchRecList
     *  @param  chn[]:      同FetchRecList
     *  @param  type:       录像类型
     *  @param  startTime:  录像开始时间，GMT时间（零时区）
     *  @param  endTime:    录像结束时间，GMT时间（零时区）
     *  @param  quality:    录像文件的清晰度，用做多个码流扩展
     *  @param  open_type   录像打开方式:0 回放, 1 下载
     *  @return recCtx:     返回打开的录像文件上下文，用做接下来的读取，关闭动作
     */
    void* (*OnOpenRecFile2)(void *ctx, int chnCnt, char chn[], int type, time_t startTime, time_t endTime, int quality, int open_type);
    /**
     *  关闭录像文件事件
     *  @param  recCtx:         录像点播上下文
     */
    int (*OnCloseRecFile)(void *recCtx);
    /**
     *  读取录像文件帧事件，深拷贝
     *  @param  recCtx:         录像点播上下文
     *  @param  pFrameHead:     录像音视频帧头
     *  @param  pRawFrame:      裸帧缓存
     *  @param  maxRawFrameSz:  裸帧缓存大小
     *  返回读取到的字节给SDK, 没有数据就返回0, 其他错误返回-1
     */
    ssize_t (*OnReadRecFrame)(void *recCtx, struct P2PFrameHead* pFrameHead, void *pRawFrame, size_t maxRawFrameSz);

    /*****************************SDK 远程设置接口****************************/
    /**
     *  远程设置事件
     *  @param setReq:     设置请求报文
     *  @param reqSz:      设置请求报文长度
     *  @param setResp:    设置应答缓存
     *  @param maxRespSz:  最大应答缓存大小
     *  @return respSz:    返回setResp大小
     */
    ssize_t (*OnRemoteSetup)(void *ctx, const char *setReq, size_t reqSz, void *setResp, size_t maxRespSz);

    /*****************************SDK 语音对讲接口****************************/
    /**
     * 语音对讲拨号事件
     * @param  ctx:         用户上下文
     * @param  talkSession: 语音对讲会话
     * @param  talkCHn:     语音对讲通道
     * @param  errNo:       失败时, 如果是设备正忙 errNo = -1, 否则填入 -2;
     * @return !=NULL:      拔通, 语音对讲业务上下文,用做接收语音数据,挂断语音对讲
     *          NULL:       失败
     */
    void*  (*OnVoP2PCall)(void *ctx, void* talkSession, int talkChn, int *errNo);
    /**
     * 语音对讲数据事件,在拔通之后就一直会收到数据,直到挂断
     * @param   talkCtx:    语音对讲上下文
     * @param   pVoiceHead: 语音信息头,共用直播,回放的属性头的音频属性
     * @param   voiceData:  语音信息
     * @param   voiceDataSz:语音数据大小/Byte
     * @return  0:          用户层处理成功
     *          -1:         用户层处理失败
     */
    ssize_t (*OnVoP2PTalkRecv)(void *talkCtx, struct P2PFrameHead* pVoiceHead, void *voiceData, size_t voiceDataSz);
    /**
     * 语音对讲挂断事件
     * @param   talkCtx:        语音对讲上下文
     */
    int (*OnVoP2PHangup)(void *talkCtx);
    /**
     * 警报拉取事件
     * @param ctx:  用户上下文
     * @param pullSession: 当前Pull业务会话, P2PSDKSendAlarmMsg发送数据时使用
     * @param chn:  请求通道
     * @param ts:   请求时间戳, 用做标识定位消息主体
     * @param type: 消息类型, 参考:P2P_ALARMMSG_TYPE_XXX
     * @return 0:  成功,应用层处理的上下文
     *         -1: 无此资源
     *         -2: 其它错误
     */
    int (*OnPullAlarmMsg)(void *ctx, void *pullSession, int chn, time_t ticket, int type);
    /**
     * @brief 连接建立事件
     * @param ctx           用户上下文
     * @param session       连接会话
     */
    void (*OnConnected)(void *ctx, void *session);
    /**
     * @brief 连接断开事件
     * @param ctx           用户上下文
     * @param session       连接会话
     */
    void (*OnClosed)(void *ctx, void *session);
    /**
     * @brief 带宽更改事件
     * @param ctx           用户上下文
     * @param session       连接会话
     * @param bandwitdh     当前会话带宽
     */
    void (*OnBindwidthChanged)(void *ctx, void *session, float bandwitdh);
};

 
    
/**
 *  设置SDK调试信息级别
 *
 *  @param logLevel 调试信息级别
 */

#define P2PSDK_EMERG     0
#define P2PSDK_CRIT      1
#define P2PSDK_ALERT     2
#define P2PSDK_ERR       3
#define P2PSDK_WARN      4
#define P2PSDK_NOTICE    5
#define P2PSDK_INFO      6
#define P2PSDK_TRACE     7
    
void P2PSDKSetLogLevel(int logLevel);


/**
 *  P2P SDK 版本号获取，返回版本字符串
 *
 *  @return 版本号字符串
 */
const char* P2PSDKGetVersion(void);


/**
 * @brief P2PSDKSetLogPath
 * @param path              日志文件路径
 * @param file_size         日志文件最大大小(byte)
 */
void P2PSDKSetLogPath(const char *path, int file_size);


/**
 *  初始化 P2PSDK
 *
 *  @param pDevice      抽象设备句柄，用于初始化SDK *
 *  @return 0:              成功
 *          -1：            失败
 */
int P2PSDKInit(struct P2PSDKDevice *pDevice);


/**
 * @brief 初始化SDK第2版
 * @param pDevice[in]       抽象设备句柄，用于初始化SDK
 * @param max_conn          设备在LTCP,TUTK传输层的最大连接数
 * @return  0               成功
 *          -1              失败
 */
int P2PSDKInit2(struct P2PSDKDevice *pDevice, int max_conn);


/**
 * 销毁 P2PSDK
 */
int P2PSDKDeinit(void);
    
/**
 *  更新设备信息, 不会更新cam_des 字段
 *
 *  @param pInfo 设备信息,
 *  @return 0       成功
 *          -1      失败
 */
int P2PSDKUpdateDevinfo(struct DeviceInfo *pInfo);

/**
 * 设置通道摄像头描述信息
 *
 * @param chn 摄像头对应通道
 * @param des 摄像头描述信息
 * @return 0    成功
 *        -1    失败
 */
int P2PSDKSetCamDes(int chn, const char *des);
    

// 透明通道服务sock类型
#define SOUP_VCON_TCP 0
#define SOUP_VCON_UDP 1
#define SOUP_VCON_UNIX_STREAM 2  //unix domain socket stream
#define SOUP_VCON_UNIX_DGRAM 3 //unix domain socket DGRAM

/**
 *  服务地址生成器
 * 生成服务地址, 返回 struct sockaddr填入app_service_addr
 * 一般 127.0.0.1:80 相应地址信息填入即可
 */
typedef int (*fAddrGenerator)(struct sockaddr *app_service_addr);


/**
 *  用户添加自己的本地服务到透明通道
 *
 *  @param app_name       服务app名称 eg, "spook"
 *  @param type           本地服务的sock类型
 *  @param addr_generator 服务地址生成器,生成服务的地址信息,保存成sockaddr形式
 *  @return 0:              成功
 *          -1：            失败
 *          调用列子:P2PSDKAddVconApp("spook", SOUP_VCON_TCP, SPOOK_AddrGenerator);
 */

int P2PSDKAddVconApp(const char *app_name, int type, fAddrGenerator addr_generator);



/**
 *  获取Esee服务器地址
 *
 *  @param  svrip   服务器地址
 *  @param  svrport 服务器端口
 *  @return 0  成功
 *          -1 失败
 */
int P2PSDKGetEseeServerAddr(char **svrip, unsigned short* svrport);
/**
 *  设置Esee服务器地址
 *
 *  @param  svrip   服务器地址
 *  @param  svrport 服务器端口
 *  @return 0  成功
 *          -1 失败
 */
int P2PSDKSetEseeServerAddr(const char *svrip, unsigned short svrport);

/**
 *  设置p2p发送缓存大小
 *
 *  @param bufsz 缓存大小/字节
 *  @return 0   成功
 *          -1  失败
 */
int P2PSDKSetSndBuf(int bufsz);

/**
 *  语音对讲发送接口
 *
 *  @param talkSession 语音对讲会话，从OnVoP2PCall中获取到
 *  @param pVoiceHead  语音数据头, <时间戳，大小，音频属性必须填充有值>
 *  @param voiceData   语音数据
 *  @param voiceDataSz 语音数据大小
 *
 *  @return 成功发送的字节
 */
ssize_t P2PSDKVoP2PTalkSend(void *talkSession, struct P2PFrameHead* pVoiceHead, void *voiceData, size_t voiceDataSz);

/**
 * 警报发送接口
 *
 * @param pullSession:  警报拉取会话
 * @param pMsgHead:     消息头 <视频宽,高,编码必须填充有值>
 * @param msgData:      消息数据
 * @param msgDataSz:    消息长度
 * @return  成功发送的字节
 */
ssize_t P2PSDKSendAlarmMsg(void *pullSession, struct P2PFrameHeadV2 *pMsgHead, void *msgData,  size_t msgDataSz);

#ifdef __cplusplus
}
#endif



#endif  //endof P2PSDK_H
