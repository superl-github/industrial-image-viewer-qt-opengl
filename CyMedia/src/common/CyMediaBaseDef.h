/**
 * @file CyMediaBaseDef.h
 * @brief CyMedia 公共基础定义的聚合头。
 * @ingroup Common
 *
 * @details
 *  按需包含时优先使用更小的头文件：
 *  - `common/ImageTypes.h`：像素格式、图像信息、色彩处理参数
 *  - `common/ParseTypes.h`：文件后缀、解析结果、视频帧信息/回调
 *  - `common/Log.h`：日志级别与回调
 *  - `common/Language.h`：界面语言
 *  - `common/Version.h`：版本号
 *  - `common/Export.h`：导入/导出宏
 *
 *  本文件聚合上述定义，便于业务代码一次性引入。
 */
#pragma once

#include "common/Export.h"
#include "common/Version.h"
#include "common/Language.h"
#include "common/Log.h"
#include "common/ImageTypes.h"
#include "common/ParseTypes.h"
