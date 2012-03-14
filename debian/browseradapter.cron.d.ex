#
# Regular cron jobs for the browseradapter package
#
0 4	* * *	root	[ -x /usr/bin/browseradapter_maintenance ] && /usr/bin/browseradapter_maintenance
