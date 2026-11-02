/**
 * @file CyMediaBaseDef.h
 * @brief CyMedia 公共基础定义的聚合头。
 * @ingroup Common
 *
 * @details
 *  按需包含时优先使用更小的头文件：
 *  - `./ImageTypes.h`：像素格式、图像信息、色彩处理参数
 *  - `./ParseTypes.h`：文件后缀、解析结果、视频帧信息/回调
 *  - `./Log.h`：日志级别与回调
 *  - `./Language.h`：界面语言
 *  - `./Version.h`：版本号
 *  - `./Export.h`：导入/导出宏
 *
 *  本文件聚合上述定义，便于业务代码一次性引入。
 */
#pragma once

#include "./Export.h"
#include "./Version.h"
#include "./Language.h"
#include "./Log.h"
#include "./ImageTypes.h"
#include "./ParseTypes.h"
