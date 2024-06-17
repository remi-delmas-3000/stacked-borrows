mutable_fail:
	cbmc --pointer-check --bounds-check --slice-formula mutable_fail.c

mutable_pass:
	cbmc --pointer-check --bounds-check --slice-formula mutable_pass.c

raw_fail:
	cbmc --pointer-check --bounds-check --slice-formula raw_fail.c

raw_pass:
	cbmc --pointer-check --bounds-check --slice-formula raw_pass.c

shared_fail:
	cbmc --pointer-check --bounds-check --slice-formula shared_fail.c

shared_pass:
	cbmc --pointer-check --bounds-check --slice-formula shared_pass.c

transmute_fail:
	cbmc --pointer-check --bounds-check --slice-formula transmute_fail.c

test:
	cbmc --pointer-check --bounds-check --slice-formula test.c

# demonic versions

mutable_fail_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula mutable_fail.c

mutable_pass_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula mutable_pass.c

raw_fail_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula raw_fail.c

raw_pass_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula raw_pass.c

shared_fail_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula shared_fail.c

shared_pass_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula shared_pass.c

transmute_fail_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula transmute_fail.c

test_demonic:
	cbmc -DDEMONIC --pointer-check --bounds-check --slice-formula test.c
