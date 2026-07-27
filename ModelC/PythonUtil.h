#ifndef PYTHONUTIL_H
#define PYTHONUTIL_H

#include <QString>

/// 找到系统中可用的 Python 命令，返回命令名称（空表示未找到）
/// 依次尝试：python → py → python3，缓存结果避免重复探测
QString findPython();

/// 将包含中文/非 ASCII 字符的路径转为纯 ASCII 临时路径
/// Python 子进程在 Windows 上可能无法正确处理非 ASCII 路径
/// 返回拷贝后的临时文件路径；若路径已是纯 ASCII 则直接返回原路径
QString sanitizePathForPython(const QString &originalPath);

#endif // PYTHONUTIL_H
