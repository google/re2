#include <re2/re2.h>
#include <re2/filtered_re2.h>
#include <stdio.h>

using namespace re2;

int main(void) {
	FilteredRE2 f;
	int id;
	f.Add("a.*b.*c", RE2::DefaultOptions, &id);
	vector<string> v;
	f.Compile(&v);

	if(RE2::FullMatch("axbyc", "a.*b.*c")) {
		printf("PASS\n");
		return 0;
	}
	printf("FAIL\n");
	return 2;
}
