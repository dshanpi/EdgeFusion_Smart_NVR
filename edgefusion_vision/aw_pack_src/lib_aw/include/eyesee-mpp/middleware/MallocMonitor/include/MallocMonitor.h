#ifndef _MALLOCMONITOR_H_
#define _MALLOCMONITOR_H_

#ifdef __cplusplus
extern "C" {
#endif

void ShowMallocStatistics();
void ShowUnreleasedMemoryInfo();
void CheckMemoryOverflow();

int StartOverflowReportThread(int nIntervalMs);
int StopOverflowReportThread();

#ifdef __cplusplus
}
#endif

#endif  /* _MALLOCMONITOR_H_ */

