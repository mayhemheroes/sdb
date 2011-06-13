#include "sdb.h"
#include "types.h"

int sdb_nexists (sdb *s, const char *key) {
	char c, *o = sdb_get (s, key);
	if (!o) return 0;
	c = *o;
	free (o);
	return c>='0' && c<='9';
}

static void strrev(char *s, int len) {
	int i, j = len -1;
	for (i=0; i<j; i++, j--) {
		char c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static void ulltoa(ut64 n, char *s) {
	int i = 0;
	do s[i++] = n % 10 + '0';
	while ((n /= 10) > 0);
	s[i] = '\0';
	strrev (s, i);
}

ut64 sdb_getn(sdb *s, const char *key) {
	ut64 n;
	char *p, *v = sdb_get (s, key);
	if (!v) return 0LL;
	n = strtoull (v, &p, 10);
	if (!p) return 0LL;
	sdb_setn (s, key, n);
	free (v);
	return n;
}

void sdb_setn(sdb *s, const char *key, ut64 v) {
	char b[128];
	ulltoa (v, b);
	sdb_set (s, key, b);
}

ut64 sdb_inc(sdb *s, const char *key, ut64 n2) {
	ut64 n = sdb_getn (s, key);
	if ((UT64_MAX-n2)<n)
		return 0LL;
	sdb_setn (s, key, n+n2);
	return n;
}

ut64 sdb_dec(sdb *s, const char *key, ut64 n2) {
	ut64 n = sdb_getn (s, key);
	if (n2>n) {
		sdb_setn (s, key, 0LL);
		return 0LL;
	}
	sdb_setn (s, key, n-n2);
	return n;
}
