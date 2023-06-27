import os
import sys
import re
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

log_file = sys.argv[1]

if not os.path.exists(log_file):
	print("FILE NOT EXIST!!")
	quit()

with open(log_file) as file:
    all_of_it = file.read()

lines = re.split(r'Current execution parameters:[.\n]*?', all_of_it)

lines = lines[1:]

get_exec_params_values = re.compile(r'''\ \ Num\ lookup:\ (?P<Num_lookup>\d+)\n
									\ \ Mini\ batch\ size\ :\ (?P<Mini_batch_size>\d+)\n
								 ''', re.X)

split_profile_info_by_action = re.compile(r'\[pnmlib profile\] Action: ([\w]+)\n((?:(?!\[pnmlib profile\] Action).*\n)*)')

get_type_measurement_and_value = re.compile(r'\[pnmlib profile\]\s*(\w+[\w\s\-]*\w)\s*(?:\([\w\s]*\))?:\s*(\d+\.?\d*)\n')


d = {
	'Num_lookup' : [],
	'Mini_batch_size' : []
}

for line in lines:
	m1 = re.search(get_exec_params_values, line)
	if m1 == None:
		print("INCORRECT FILE FORMAT!!")
		quit()
	d['Num_lookup'].append(int(m1.group('Num_lookup')))
	d['Mini_batch_size'].append(int(m1.group('Mini_batch_size')))
	for (action, times) in re.findall(split_profile_info_by_action, line):
		d.setdefault(action, []).append(int(dict(re.findall(get_type_measurement_and_value, times))['Root-mean-square duration']))

d['num_inst'] = [i1 * i2 for i1, i2 in zip(d['Num_lookup'], d['Mini_batch_size'])]

df = pd.DataFrame(data=d)

sns.set_theme()

sns.set_style("whitegrid")
sns.relplot(
    data=df,
    x="num_inst", y="axdimm_run_sls_impl", hue="Num_lookup", palette = ['tab:red', 'tab:orange', 'xkcd:yellow', 'tab:green', 'tab:blue', 'xkcd:black'])
plt.savefig('axdimm_run_sls_impl-hue_Num_lookup.pdf')

sns.set_theme()
sns.set_style("whitegrid")
sns.relplot(
    data=df,
    x="num_inst", y="axdimm_run_sls_impl", hue="Mini_batch_size")
plt.savefig('axdimm_run_sls_impl-hue_Mini_batch_size.pdf')

sns.set_style("whitegrid")
sns.relplot(
    data=df,
    x="num_inst", y="write_inst_block", hue="Num_lookup", palette = ['tab:red', 'tab:orange', 'xkcd:yellow', 'tab:green', 'tab:blue', 'xkcd:black'])
plt.savefig('write_inst_block-hue_Num_lookup.pdf')

sns.set_style("whitegrid")
sns.relplot(
    data=df,
    x="num_inst", y="write_inst_block", hue="Mini_batch_size")
plt.savefig('write_inst_block-hue_Mini_batch_size.pdf')

sns.set_style("whitegrid")
sns.relplot(
    data=df,
    x="Mini_batch_size", y="read_and_reset_psum", hue="Num_lookup", palette = ['tab:red', 'tab:orange', 'xkcd:yellow', 'tab:green', 'tab:blue', 'xkcd:black'])
plt.savefig('read_and_reset_psum-hue_Num_lookup.pdf')
