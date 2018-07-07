#ifndef CAPABILITY_SET_H_
#define CAPABILITY_SET_H_

/**
 * fisheye类型
 */
typedef enum
{
    emCAPABILITY_SET_FISHEYE_TYPE_NONE = 0,
    emCAPABILITY_SET_FISHEYE_TYPE_WALL = 180,
    emCAPABILITY_SET_FISHEYE_TYPE_CELL = 360,
    emCAPABILITY_SET_FISHEYE_TYPE_TABLE = 720
}emCAPABILITY_SET_FISHEYE_TYPE;

typedef struct CAPABILITY_SET
{
    unsigned int version;        // 能力集版本，不能缺省，每次版本迭代递增
    unsigned int maxChannel;     // 缺省 1
    unsigned int lightControl;   // 缺省 0, 选项 1,2,3,4
    unsigned int bulbControl;    // 缺省 0, 选项 1,2,3,4
    unsigned int fisheye;        // 缺省 0, 180,360,720
    bool ptz;           // 缺省 true
    bool sdCard;        // 缺省 true
    bool lte;           // 缺省 false
    bool wifi;          // 缺省 true
    bool rtc;           // 缺省 false
    bool rj45;          // 缺省 false
    bool powerBattery;  // 缺省 false
    bool audioInput;    // 缺省 true
    bool audioOutput;   // 缺省 true
    bool bluetooth;     // 缺省 false
}stCAPABILITY_SET, *pstCAPABILITY_SET;

/**
 * 能力集配置文件查找及解析到内存结构体
 * @return 0成功|-1失败
 */
extern int CAPABILITY_SET_init();

/**
 * 能力集已初始化判断
 * @return true成功|false失败
 */
bool CAPABILITY_SET_is_inited();

/**
 * 能力集数据结构体获取
 * @param  capabilitySet 能力集数据结构体
 * @return        0成功|-1失败
 */
extern int CAPABILITY_SET_get(pstCAPABILITY_SET capabilitySet);

#endif /* CAPABILITY_SET_H_ */

