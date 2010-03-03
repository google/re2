#include <re2/re2.h>
#include <stdio.h>

using namespace re2;

int main(void) {
	if(RE2::FullMatch("axbyc", "a.*b.*c")) {
		printf("PASS\n");
		return 0;
	}
	printf("FAIL\n");
	return 2;
}
