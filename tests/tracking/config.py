# Copyright 2011, Kitware, Inc.
# See `LICENSE' file for licensing information.

""" This ConfigBlock class reads a vidtk-style configuration file

 It should be used in this manner:

 If your config file, myconfig.conf, looked like:
   param1 = value1
   param2 = value2
   sub_block1:param3 = value3
   sub_block1:param4 = value4
   sub_block1:sub_block2:param5 = value5

 then you would parse the file like this:
   config = ConfigBlock()
   config.parse("myconfigfile.conf")

 and you would read the parameters one of several ways:
   param1 = config["param1"]
   param2 = config["param2"]
   param3 = config["sub_block1:param3"]
   param4 = config["sub_block1:param4"]
   param5 = config["sub_block1:sub_block2:param5"]

 or
   param1 = config["param1"]
   param2 = config["param2"]
   param3 = config["sub_block1"]["param3"]
   param4 = config["sub_block1"]["param4"]
   param5 = config["sub_block1"]["sub_block2"]["param5"]

 or
   param1 = config["param1"]
   param2 = config["param2"]
   config_sub1 = config["sub_block1"]
   param3 = config_sub1["param3"]
   param4 = config_sub1["param4")
   config_sub2 = config_sub1["SubBlock2"]
   param5 = config_sub2["param5"]
"""

import re

class ConfigBlock(object):
    """Represents a collection of nested configuation blocks.
    """
    def __init__(self):
        self.values = {}
        self.blocks = {}


    def __getitem__(self, key):
        """Access a value or block by name
        """
        parts = key.split(':',1)
        if len(parts) == 2:
            block_name, block_key = parts
            if block_name not in self.blocks:
                raise KeyError
                return None
            return self.blocks[block_name][block_key]

        if key in self.values:
            return self.values[key]
        elif key in self.blocks:
            return self.blocks[key]
        raise KeyError
        return None


    def __setitem__(self, key, value):
        """Set a value or block by name

        examples:
          config_block['value'] = 1.5
          config_block['sub_block:value'] = "name"
          config_block['sub_block']['value'] = "name"
        """
        parts = key.split(':',1)
        if len(parts) == 2:
            block_name, block_key = parts
            if block_name not in self.blocks:
                # create new sub blocks if needed
                self.blocks[block_name] = ConfigBlock()
            self.blocks[block_name][block_key] = value
            return

        if isinstance(value, type(self)):
            self.blocks[key] = value
        else:
            self.values[key] = str(value)


    def __delitem__(self, key):
        """Delete a value or block by name
        """
        parts = key.split(':',1)
        if len(parts) == 2:
            block_name, block_key = parts
            if block_name not in self.blocks:
                raise KeyError
                return None
            del(self.blocks[block_name][block_key])
            return

        if key in self.values:
            del(self.values[key])
        elif key in self.blocks:
            del(self.blocks[key])
        else:
            raise KeyError


    def __contains__(self, key):
        """Access a value or block by name
        """
        parts = key.split(':',1)
        if len(parts) == 2:
            block_name, block_key = parts
            if block_name not in self.blocks:
                return False
            return block_key in self.blocks[block_name]

        if key in self.values:
            return True
        elif key in self.blocks:
            return True
        return False


    def __repr__(self):
        """Produce a string representation of the config block.
        """
        return self.make_string(nested=False)


    def __str__(self):
        """Produce a string representation of the config block.
        """
        return self.make_string(nested=True)


    def make_string(self, nested=False, indent='  '):
        """Produce a string representation of the config block.
        """
        def prefix_lines(text, pre):
            """Prefix each line in text with pre."""
            lines = []
            for line in text.splitlines(True):
                if line.strip():
                    lines.append(pre)
                lines.append(line)
            return ''.join(lines)

        rep = ""
        for name, val in sorted(self.values.items()):
            rep += name + " = " + val + "\n"
        if len(self.values) > 0 and len(self.blocks) > 0:
            rep += "\n"
        for name, block in sorted(self.blocks.items()):
            if nested:
                rep += "block %s\n" % name
                rep += prefix_lines(block.make_string(nested=nested,
                                                      indent=indent), indent)
                rep += "endblock\n"
            else:
                rep += prefix_lines(block.make_string(nested=nested,
                                                      indent=indent), name+':')
            rep += '\n'
        # ensure only a single terminated newline
        rep = rep.rstrip('\n')+'\n'
        return rep


    def parse(self, filename):
        """Load the config block from a config file.
        """
        f = open(filename, "r")
        return self.parse_stream(f)


    def parse_stream(self, s):
        """Load the config block from an existing stream.
        """
        while 1 :
            line = s.readline()
            if not line :
                break
            line = line.strip()

            # Skip empty lines and comments
            if len(line) == 0 or re.match("^#", line) != None :
                continue

            # Process top level starting block
            block_match = re.match("^block (.+)", line)
            if block_match != None :
                block_name = block_match.group(1)
                if block_name in self.blocks :
                    block = self.blocks[block_name]
                else:
                    block = ConfigBlock()
                    self.blocks[block_name] = block
                if not block.parse_stream(s) :
                    print "Error: failure parsing block", block_name
                    return False
                continue

            # Process top level ending block
            if line == "endblock" :
                return True

            # Process parameter entries
            match_param = re.match("([^=]+)=(.+)", line)
            if match_param != None :
                param_name = match_param.group(1).strip()
                param_value = match_param.group(2).strip()
                self[param_name] = param_value
                continue

            print "Warning: unable to parse '" + line + "'"
        return True

    def __iter__(self):
        def prefix(name, d):
            nd = {}

            for k, v in d:
                nd['%s:%s' % (name, k)] = v

            return nd

        def mkdict(blk):
            d = blk.values

            for name, block in blk.blocks:
                d = dict(d.items() + prefix(name, mkdict(block)))

            return d

        vals = mkdict(self)

        return iter(vals)
