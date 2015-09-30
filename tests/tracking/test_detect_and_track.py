#1/usr/bin/env python


from datetime import datetime
from os.path import join


import os
import shutil
import subprocess
import sys


from config import ConfigBlock


def get(config, key, func=None):
    try:
        val = config[key]

        if func is not None:
            val = func(val)

        return val
    except:
        return None


def main(bindir, confdir, videopath, confname, trkconfname, outdir, resdir):
    succeed = True

    confpath = join(confdir, confname)

    with open(confpath, 'r') as conffile:
        try:
            config = ConfigBlock()
            config.parse_stream(conffile)

            tsucceed = True
        except:
            tsucceed = False

    # If the last test failed, we can't reliably
    # continue with the other tests...
    if tsucceed:
        print("PASS: Read test configuration file")
    else:
        print("FAIL: Read test configuration file")

        succeed = False
        return succeed

    trkconfpath = join(confdir, trkconfname)

    # Arguments for the test
    exepath = get(config, 'exe', lambda p: join(bindir, p))

    # Output file
    track_file = get(config, 'track_file')

    # Expected outputs
    ex_num_tracks = get(config, 'num_tracks', int)
    ex_num_tracks_cmp = get(config, 'num_tracks_cmp') or 'eq'
    ex_track_score = get(config, 'track_score', int)
    ex_track_score_cmp = get(config, 'track_score_cmp') or 'eq'
    ex_homog_file = get(config, 'homog_file')
    ex_homog_tolerance = get(config, 'homog_tolerance')
    ex_conn_comp_dir = get(config, 'conn_comp_dir')
    ex_timestamps = get(config, 'timestamps')
    ex_modality = get(config, 'modality')
    ex_num_shot_breaks = get(config, 'num_shot_breaks')
    ex_shot_break_frames = get(config, 'shot_break_frames')
    ex_lag_length = get(config, 'lag_length')

    # If the last test failed, we can't reliably
    # continue with the other tests...
    if exepath is not None:
        print("PASS: Test preparation")
    else:
        print("FAIL: Test preparation")

        succeed = False
        return succeed

    try:
        os.removedirs(outdir)
    except:
        pass

    try:
        os.makedirs(outdir)
    except:
        pass

    try:
        conf = []
        if os.path.exists(videopath):
            conf = ['detect_and_track:src:vidl_ffmpeg:filename=%s' % videopath,
                    'detect_and_track:src:type=vidl_ffmpeg']

        retcode = subprocess.call([exepath,
                                   '-c', trkconfpath] + conf,
                                  cwd=outdir)

        if not retcode:
            tsucceed = True
        else:
            tsucceed = False
    except:
        print("FAIL: Executing tracker")

        succeed = False
        return succeed

    # If the last test failed, we can't reliably
    # continue with the other tests...
    if tsucceed:
        print("PASS: Running tracker")
    else:
        print("FAIL: Running tracker")

        succeed = False
        return succeed

    def eq(res, expect):
        return res == expect

    def lt(res, expect):
        return res < expect

    def gt(res, expect):
        return res > expect

    def le(res, expect):
        return res <= expect

    def ge(res, expect):
        return res >= expect

    def percent(percent):
        import math

        def f(res, expect):
            return (math.fabs(res - expect) / expect) <= percent

        return f

    cmps = { 'eq': eq,
             'lt': lt,
             'gt': gt,
             'le': le,
             'ge': ge,
             '1%': percent(0.01),
             '5%': percent(0.05),
             '10%': percent(0.10),
           }

    if track_file is not None and ex_num_tracks is not None:
        track_ids = set()

        with open(join(outdir, track_file), 'r') as fin:
            for line in fin:
                if line.startswith('#'):
                    continue

                track_id = line.split(' ')[0]

                track_ids.add(track_id)

        num_tracks = len(track_ids)
        compare = cmps[ex_num_tracks_cmp]

        if not compare(num_tracks, ex_num_tracks):
            print("FAIL: Expected %s %d tracks as a result, got %d instead" % (ex_num_tracks_cmp, ex_num_tracks, num_tracks))

    if ex_track_score is not None:
        pass

    if ex_homog_file is not None:
        if ex_homog_tolerance is not None:
            pass

        pass

    # TODO: Difference images

    if ex_conn_comp_dir is not None:
        pass

    if ex_timestamps is not None:
        pass

    # TODO: Interpolation

    if ex_modality is not None:
        pass

    if ex_num_shot_breaks is not None:
        pass

    if ex_shot_break_frames is not None:
        pass

    # TODO: Shot recovery

    if ex_lag_length is not None:
        pass

    try:
        if os.path.exists(resdir):
            testresdir = join(resdir, datetime.now().strftime('%Y-%m-%d-%H.%M.%S'))
            shutil.copytree(outdir, testresdir)

        tsucceed = True
    except Exception as e:
        print e
        tsucceed = False

    if tsucceed:
        print("PASS: Copying results")
    else:
        print("FAIL: Copying results")

        succeed = False

    return succeed


if __name__ == '__main__':
    print sys.argv
    if len(sys.argv) < 8:
        print("Usage: %s <bindir> <confdir> <videopath> <confname> <trkconfname> <outdir> <resdir>" % sys.argv[0])

        sys.exit(1)

    bindir = sys.argv[1]
    confdir = sys.argv[2]
    videopath = sys.argv[3]
    confname = sys.argv[4]
    trkconfname = sys.argv[5]
    outdir = sys.argv[6]
    resdir = sys.argv[7]

    if main(bindir, confdir, videopath, confname, trkconfname, outdir, resdir):
        sys.exit(0)
    else:
        sys.exit(1)
