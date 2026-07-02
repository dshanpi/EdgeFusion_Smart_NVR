#ifndef EDGEFUSION_WEB_H
#define EDGEFUSION_WEB_H

#include "conf.h"

/* Start the embedded HTTP server.
 * Endpoints:
 *   GET  /                  Static: web_root/index.html
 *   GET  /stream            MJPEG live preview
 *   GET  /api/status        Aggregated status JSON
 *   GET  /api/snapshot      Single JPEG snapshot
 *   GET  /api/recordings    MP4 recording list JSON
 *   GET  /api/events        Event list JSON (?limit=&offset=&type=)
 *   GET  /api/event/<id>    Single event detail JSON
 *   GET  /api/config        Current config JSON (sensitive fields masked)
 *   POST /api/config        Update config (JSON body), save + apply hot-reload
 *   GET  /rec/<name>        Serve MP4 recording with Range support
 *   GET  /snap/evt_<id>.jpg Serve event snapshot JPEG
 *   GET  /<path>            Static files from web_root
 */
int  web_start(const gateway_conf_t *cfg);
void web_stop(void);

#endif
