/* prepare to install the MMIO tracing hooks (allocate memory etc) */
void io_trace_prepare();

/* install the MMIO tracing hooks */
/* can be done with interrupts disabled, if desired */
void io_trace_install();

/* uninstall the MMIO tracing hooks; logging buffer is still available for reading */
void io_trace_uninstall();

/* free the logging buffer, completing the uninstallation */
void io_trace_cleanup();

/* dump the MMIO data to a log file */
void io_trace_dump();

#ifdef CONFIG_MMIO_TRACE
/* return its (next) index, for syncing with dm-spy (from 0 to N-1, consecutive) */
uint32_t io_trace_log_get_index();

/* return maximum number of MMIO entries */
uint32_t io_trace_log_get_nmax();

/* log message given by index (will be called from 0 to what the above returned - 1) */
/* note: msg_buffer and msg_size can be either used directly, or passed to debug_format_msg */
/* extra care required if you want to mix these two methods */
int io_trace_log_message(uint32_t msg_index, char * msg_buffer, int msg_size);

/* get timer value without logging it as MMIO access */
uint32_t io_trace_get_timer();

/* temporarily pause/resume MMIO logging */
void io_trace_pause();
void io_trace_resume();

#else /* dummy stubs */
static uint32_t io_trace_log_get_index() { return 0xFFFFFFFF; }
static uint32_t io_trace_log_get_nmax() { return 0; }
static uint32_t io_trace_get_timer() { return MEM(0xC0242014); }
static void io_trace_pause() { }
static void io_trace_resume() { }
#endif
