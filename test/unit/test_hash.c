#include "minunit.h"
#include <sdb.h>
#include <ht_uu.h>
#include <ht_up.h>
#include <ht_pp.h>

typedef struct _test_struct {
	char *name;
	int age;
} Person;

bool test_ht_insert_lookup(void) {
	HtPP *ht = sdb_ht_new ();
	sdb_ht_insert (ht, "AAAA", "vAAAA");
	sdb_ht_insert (ht, "BBBB", "vBBBB");
	sdb_ht_insert (ht, "CCCC", "vCCCC");

	mu_assert_streq (sdb_ht_find (ht, "BBBB", NULL), "vBBBB", "BBBB value wrong");
	mu_assert_streq (sdb_ht_find (ht, "AAAA", NULL), "vAAAA", "AAAA value wrong");
	mu_assert_streq (sdb_ht_find (ht, "CCCC", NULL), "vCCCC", "CCCC value wrong");

	sdb_ht_free (ht);
	mu_end;
}

bool test_ht_update_lookup(void) {
	HtPP *ht = sdb_ht_new ();
	sdb_ht_insert (ht, "AAAA", "vAAAA");
	sdb_ht_insert (ht, "BBBB", "vBBBB");

	// test update to add a new element
	sdb_ht_update (ht, "CCCC", "vCCCC");
	mu_assert_streq (sdb_ht_find (ht, "CCCC", NULL), "vCCCC", "CCCC value wrong");

	// test update to replace an existing element
	sdb_ht_update (ht, "AAAA", "vDDDD");
	mu_assert_streq (sdb_ht_find (ht, "AAAA", NULL), "vDDDD", "DDDD value wrong");

	sdb_ht_free (ht);
	mu_end;
}

bool test_ht_delete(void) {
	HtPP *ht = sdb_ht_new ();
	mu_assert ("nothing should be deleted", !sdb_ht_delete (ht, "non existing"));

	sdb_ht_insert (ht, "AAAA", "vAAAA");
	mu_assert ("AAAA should be deleted", sdb_ht_delete (ht, "AAAA"));
	mu_assert ("AAAA still there", !sdb_ht_find (ht, "AAAA", NULL));

	sdb_ht_free (ht);
	mu_end;
}

bool test_ht_insert_kvp(void) {
	HtPP *ht = sdb_ht_new ();
	SdbKv *kv = sdbkv_new ("AAAA", "vAAAA");
	mu_assert ("AAAA shouldn't exist", !sdb_ht_find_kvp (ht, "AAAA", NULL));
	sdb_ht_insert_kvp (ht, kv, false);
	free (kv);

	mu_assert ("AAAA should exist", sdb_ht_find_kvp (ht, "AAAA", NULL));
	SdbKv *kv2 = sdbkv_new ("AAAA", "vNEWAAAA");
	mu_assert ("AAAA shouldn't be replaced", !sdb_ht_insert_kvp (ht, kv2, false));
	mu_assert ("AAAA should be replaced", sdb_ht_insert_kvp (ht, kv2, true));
	free (kv2);

	SdbKv *foundkv = sdb_ht_find_kvp (ht, "AAAA", NULL);
	mu_assert_streq (foundkv->base.value, "vNEWAAAA", "vNEWAAAA should be there");

	sdb_ht_free (ht);
	mu_end;
}

ut32 create_collision(const void *key) {
	return 10;
}

bool test_ht_insert_collision(void) {
	HtPP *ht = sdb_ht_new ();
	ht->opt.hashfn = create_collision;
	ht_pp_insert (ht, "AAAA", "vAAAA");
	mu_assert_streq (sdb_ht_find (ht, "AAAA", NULL), "vAAAA", "AAAA should be there");
	ht_pp_insert (ht, "BBBB", "vBBBB");
	mu_assert_streq (sdb_ht_find (ht, "AAAA", NULL), "vAAAA", "AAAA should still be there");
	mu_assert_streq (sdb_ht_find (ht, "BBBB", NULL), "vBBBB", "BBBB should be there");
	ht_pp_insert (ht, "CCCC", "vBBBB");
	mu_assert_streq (sdb_ht_find (ht, "CCCC", NULL), "vBBBB", "CCCC should be there");

	sdb_ht_free (ht);
	mu_end;
}

ut32 key2hash(const void *key) {
	return atoi(key);
}

bool test_ht_grow(void) {
	HtPP *ht = sdb_ht_new ();
	char str[15], vstr[15];
	char buf[100];
	int i;

	ht->opt.hashfn = key2hash;
	for (i = 0; i < 20000; i++) {
		snprintf (str, 15, "%d", i);
		snprintf (vstr, 15, "v%d", i);
		sdb_ht_insert (ht, str, vstr);
	}

	for (i = 0; i < 20000; i++) {
		snprintf (str, 15, "%d", i);
		snprintf (vstr, 15, "v%d", i);
		char *v = sdb_ht_find (ht, str, NULL);
		snprintf (buf, 100, "%s/%s should be there", str, vstr);
		mu_assert (buf, v);
		snprintf (buf, 100, "%s/%s should be right", str, vstr);
		mu_assert_streq (v, vstr, buf);
	}

	sdb_ht_free (ht);
	mu_end;
}

bool test_ht_kvp(void) {
	HtPP *ht = sdb_ht_new ();
	SdbKv *kvp = sdbkv_new ("AAAA", "vAAAA");

	mu_assert_eq (kvp->base.key_len, 4, "key_len should be 4");
	mu_assert_eq (kvp->base.value_len, 5, "value_len should be 5");
	mu_assert ("kvp should be inserted", sdb_ht_insert_kvp (ht, kvp, false));
	free (kvp);

	kvp = sdb_ht_find_kvp (ht, "AAAA", NULL);
	mu_assert_eq (kvp->base.key_len, 4, "key_len should be 4 after kvp_insert");
	mu_assert_eq (kvp->base.value_len, 5, "value_len should be 5 after kvp_insert");

	sdb_ht_insert (ht, "BBBB", "vBBBB");
	kvp = sdb_ht_find_kvp (ht, "BBBB", NULL);
	mu_assert_eq (kvp->base.key_len, 4, "key_len should be 4 after insert");
	mu_assert_eq (kvp->base.value_len, 5, "value_len should be 5 after insert");

	sdb_ht_free (ht);
	mu_end;
}

Person* duplicate_person(Person *p) {
	Person* c = malloc (sizeof (Person));
	c->name = strdup (p->name);
	c->age = p->age;
	return c;
}

void free_kv(HtPPKv *kv) {
	free (kv->key);
	Person *p = kv->value;
	free (p->name);
	free (p);
}

size_t calcSizePerson(void *c) {
	Person *p = c;
	return sizeof (*p);
}
bool test_ht_general(void) {
	int retval = MU_PASSED;
	bool found = false;
	Person *p, *person1 = malloc (sizeof (Person));
	if (!person1) {
		mu_cleanup_fail(err_malloc, "person1 malloc");
	}
	person1->name = strdup ("radare");
	person1->age = 10;

	Person *person2 = malloc (sizeof (Person));
	if (!person2) {
		mu_cleanup_fail(err_free_person1, "person2 malloc");
	}
	person2->name = strdup ("pancake");
	person2->age = 9000;

	HtPP *ht = ht_pp_new ((HtPPDupValue)duplicate_person, free_kv, (HtPPCalcSizeV)calcSizePerson);
	if (!ht) {
		mu_cleanup_fail(err_free_persons, "ht alloc");
	}
	ht_pp_insert (ht, "radare", (void *)person1);
	ht_pp_insert (ht, "pancake", (void *)person2);
	p = ht_pp_find (ht, "radare", &found);
	mu_assert ("radare not found", found);
	mu_assert_streq (p->name, "radare", "wrong person");
	mu_assert_eq (p->age, 10, "wrong radare age");

	p = ht_pp_find (ht, "pancake", &found);
	mu_assert ("radare not found", found);
	mu_assert_streq (p->name, "pancake", "wrong person");
	mu_assert_eq (p->age, 9000, "wrong pancake age");

	(void)ht_pp_find (ht, "not", &found);
	mu_assert ("found but it should not exists", !found);

	ht_pp_delete (ht, "pancake");
	p = ht_pp_find (ht, "pancake", &found);
	mu_assert ("pancake was deleted", !found);

	ht_pp_insert (ht, "pancake", (void *)person2);
	ht_pp_delete (ht, "radare");
	ht_pp_update (ht, "pancake", (void *)person1);
	p = ht_pp_find (ht, "pancake", &found);

	mu_assert ("pancake was updated", found);
	mu_assert_streq (p->name, "radare", "wrong person");
	mu_assert_eq (p->age, 10, "wrong age");

	ht_pp_free (ht);
 err_free_persons:
	free (person2->name);
	free (person2);
 err_free_person1:
	free (person1->name);
	free (person1);
 err_malloc:
	mu_cleanup_end;
}

static void free_key_value(HtPPKv *kv) {
	free (kv->key);
	free (kv->value);
}

bool should_not_be_caled(void *user, const char *k, void *v) {
	mu_fail ("this function should not be called");
	return false;
}

bool test_empty_ht(void) {
	HtPP *ht = ht_pp_new0 ();
	ht_pp_foreach (ht, (HtPPForeachCallback) should_not_be_caled, NULL);
	void *r = ht_pp_find (ht, "key1", NULL);
	mu_assert_null (r, "key1 should not be present");
	ht_pp_free (ht);
	mu_end;
}

bool test_insert(void) {
	HtPP *ht = ht_pp_new0 ();
	void *r;
	bool res;
	bool found;

	res = ht_pp_insert (ht, "key1", "value1");
	mu_assert ("key1 should be a new element", res);
	r = ht_pp_find (ht, "key1", &found);
	mu_assert ("found should be true", found);
	mu_assert_streq (r, "value1", "value1 should be retrieved");

	res = ht_pp_insert (ht, "key1", "value2");
	mu_assert ("key1 should be an already existing element", !res);
	r = ht_pp_find (ht, "key1", &found);
	mu_assert_streq (r, "value1", "value1 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key2", &found);
	mu_assert_null (r, "key2 should not be present");
	mu_assert ("found for key2 should be false", !found);

	ht_pp_free (ht);
	mu_end;
}

bool test_update(void) {
	HtPP *ht = ht_pp_new0 ();
	bool found;

	ht_pp_insert (ht, "key1", "value1");
	ht_pp_update (ht, "key1", "value2");
	void *r = ht_pp_find (ht, "key1", &found);
	mu_assert_streq (r, "value2", "value2 should be retrieved");
	mu_assert ("found should be true", found);
	ht_pp_free (ht);
	mu_end;
}

bool test_delete(void) {
	HtPP *ht = ht_pp_new0 ();
	bool found;

	ht_pp_insert (ht, "key1", "value1");
	ht_pp_delete (ht, "key1");
	void *r = ht_pp_find (ht, "key1", &found);
	mu_assert_null (r, "key1 should not be found");
	mu_assert ("found should be false", !found);
	ht_pp_free (ht);
	mu_end;
}

static bool grow_1_found[3];
static bool grow_1_foreach(void *user, const char *k, int v) {
	grow_1_found[v] = true;
	return true;
}

bool test_grow_1(void) {
	HtPP *ht = ht_pp_new0 ();
	int i;

	for (i = 0; i < 3; i++) {
		grow_1_found[i] = false;
	}

	ht_pp_insert (ht, "key0", (void *)0);
	ht_pp_insert (ht, "key1", (void *)1);
	ht_pp_insert (ht, "key2", (void *)2);

	ht_pp_foreach (ht, (HtPPForeachCallback)grow_1_foreach, NULL);
	for (i = 0; i < 3; i++) {
		if (!grow_1_found[i]) {
			fprintf (stderr, "i = %d\n", i);
			mu_fail ("An element has not been traversed");
		}
	}

	ht_pp_free (ht);
	mu_end;
}

bool test_grow_2(void) {
	HtPP *ht = ht_pp_new ((HtPPDupValue)strdup, (HtPPKvFreeFunc)free_key_value, NULL);
	char *r;
	bool found;
	int i;

	for (i = 0; i < 3000; i++) {
		char buf[20], buf2[20];
		snprintf (buf, 20, "key%d", i);
		snprintf (buf2, 20, "value%d", i);
		ht_pp_insert (ht, buf, buf2);
	}

	r = ht_pp_find (ht, "key1", &found);
	mu_assert_streq (r, "value1", "value1 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key2000", &found);
	mu_assert_streq (r, "value2000", "value2000 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key4000", &found);
	mu_assert_null (r, "key4000 should not be there");
	mu_assert ("found should be false", !found);

	ht_pp_free (ht);
	mu_end;
}

bool test_grow_3(void) {
	HtPP *ht = ht_pp_new ((HtPPDupValue)strdup, (HtPPKvFreeFunc)free_key_value, NULL);
	char *r;
	bool found;
	int i;

	for (i = 0; i < 3000; i++) {
		char buf[20], buf2[20];
		snprintf (buf, 20, "key%d", i);
		snprintf (buf2, 20, "value%d", i);
		ht_pp_insert (ht, buf, buf2);
	}

	for (i = 0; i < 3000; i += 3) {
		char buf[20];
		snprintf (buf, 20, "key%d", i);
		ht_pp_delete (ht, buf);
	}

	r = ht_pp_find (ht, "key1", &found);
	mu_assert_streq (r, "value1", "value1 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key2000", &found);
	mu_assert_streq (r, "value2000", "value2000 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key4000", &found);
	mu_assert_null (r, "key4000 should not be there");
	mu_assert ("found should be false", !found);

	r = ht_pp_find (ht, "key0", &found);
	mu_assert_null (r, "key0 should not be there");
	mu_assert ("found should be false", !found);

	for (i = 1; i < 3000; i += 3) {
		char buf[20];
		snprintf (buf, 20, "key%d", i);
		ht_pp_delete (ht, buf);
	}

	r = ht_pp_find (ht, "key1", &found);
	mu_assert_null (r, "key1 should not be there");
	mu_assert ("found should be false", !found);

	ht_pp_free (ht);
	mu_end;
}

bool test_grow_4(void) {
	HtPP *ht = ht_pp_new (NULL, (HtPPKvFreeFunc)free_key_value, NULL);
	char *r;
	bool found;
	int i;

	for (i = 0; i < 3000; i++) {
		char buf[20], *buf2;
		snprintf (buf, 20, "key%d", i);
		buf2 = malloc (20);
		snprintf (buf2, 20, "value%d", i);
		ht_pp_insert (ht, buf, buf2);
	}

	r = ht_pp_find (ht, "key1", &found);
	mu_assert_streq (r, "value1", "value1 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key2000", &found);
	mu_assert_streq (r, "value2000", "value2000 should be retrieved");
	mu_assert ("found should be true", found);

	for (i = 0; i < 3000; i += 3) {
		char buf[20];
		snprintf (buf, 20, "key%d", i);
		ht_pp_delete (ht, buf);
	}

	r = ht_pp_find (ht, "key2000", &found);
	mu_assert_streq (r, "value2000", "value2000 should be retrieved");
	mu_assert ("found should be true", found);

	r = ht_pp_find (ht, "key0", &found);
	mu_assert_null (r, "key0 should not be there");
	mu_assert ("found should be false", !found);

	for (i = 1; i < 3000; i += 3) {
		char buf[20];
		snprintf (buf, 20, "key%d", i);
		ht_pp_delete (ht, buf);
	}

	r = ht_pp_find (ht, "key1", &found);
	mu_assert_null (r, "key1 should not be there");
	mu_assert ("found should be false", !found);

	ht_pp_free (ht);
	mu_end;
}

bool foreach_delete_cb(void *user, const ut64 key, const void *v) {
	HtUP *ht = (HtUP *)user;

	ht_up_delete (ht, key);
	return true;
}

static void free_up_value(HtUPKv *kv) {
	free (kv->value);
}

bool test_foreach_delete(void) {
	HtUP *ht = ht_up_new ((HtUPDupValue)strdup, free_up_value, NULL);

	// create a collision
	ht_up_insert (ht, 0, "value1");
	ht_up_insert (ht, ht->size, "value2");
	ht_up_insert (ht, ht->size * 2, "value3");
	ht_up_insert (ht, ht->size * 3, "value4");

	ht_up_foreach (ht, foreach_delete_cb, ht);
	ht_up_foreach (ht, (HtUPForeachCallback) should_not_be_caled, NULL);

	ht_up_free (ht);
	mu_end;
}

bool test_update_key(void) {
	bool res;
	HtUP *ht = ht_up_new ((HtUPDupValue)strdup, free_up_value, NULL);

	// create a collision
	ht_up_insert (ht, 0, "value1");
	ht_up_insert (ht, 0xdeadbeef, "value2");
	ht_up_insert (ht, 0xcafebabe, "value3");

	res = ht_up_update_key (ht, 0xcafebabe, 0x10000);
	mu_assert ("cafebabe should be updated", res);
	res = ht_up_update_key (ht, 0xdeadbeef, 0x10000);
	mu_assert ("deadbeef should NOT be updated, because there's already an element at 0x10000", !res);

	const char *v = ht_up_find (ht, 0x10000, NULL);
	mu_assert_streq (v, "value3", "value3 should be at 0x10000");
	v = ht_up_find (ht, 0xdeadbeef, NULL);
	mu_assert_streq (v, "value2", "value2 should remain at 0xdeadbeef");

	ht_up_free (ht);
	mu_end;
}

int all_tests() {
	mu_run_test (test_ht_insert_lookup);
	mu_run_test (test_ht_update_lookup);
	mu_run_test (test_ht_delete);
	mu_run_test (test_ht_insert_kvp);
	mu_run_test (test_ht_insert_collision);
	mu_run_test (test_ht_grow);
	mu_run_test (test_ht_kvp);
	mu_run_test (test_ht_general);
	mu_run_test (test_empty_ht);
	mu_run_test (test_insert);
	mu_run_test (test_update);
	mu_run_test (test_delete);
	mu_run_test (test_grow_1);
	mu_run_test (test_grow_2);
	mu_run_test (test_grow_3);
	mu_run_test (test_grow_4);
	mu_run_test (test_foreach_delete);
	mu_run_test (test_update_key);
	return tests_passed != tests_run;
}

int main(int argc, char **argv) {
	return all_tests ();
}
