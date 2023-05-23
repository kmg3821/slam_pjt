class config:
    def __init__(self, fileSrc):
        self.environments = {}
        with open(fileSrc, 'rt', encoding='utf-8') as f:
            lines = f.readlines()
            for line in lines:
                if line == '':
                    continue
                key, val = line.split('=')
                key = key.strip()
                val = val.strip()
                self.environments[key]=val
    
    def get(self, env_name):
        return self.environments[env_name]

    def list(self):
        return self.environments
