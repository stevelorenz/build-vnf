import yaml

with open("./cnf.yaml", "r") as f:
    y = yaml.load(f)
    print(y)
