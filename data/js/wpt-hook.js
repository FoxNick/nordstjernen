(function (global) {
    'use strict';
    if (global.__ns_wpt_installed) return;
    global.__ns_wpt_installed = true;
    global.__ns_wpt_done = false;
    global.__ns_wpt_seen_harness = false;

    function squash(s) {
        return String(s).replace(/[\u0000-\u001f\u007f-\u009f]+/g, ' ')
                        .replace(/\s+/g, ' ').trim();
    }

    global.__ns_wpt_oncomplete = function (tests, status) {
        if (global.__ns_wpt_done) return;
        var harnessNames = ['OK', 'ERROR', 'TIMEOUT', 'PRECONDITION_FAILED'];
        var subtestNames = ['PASS', 'FAIL', 'TIMEOUT', 'NOTRUN',
                            'PRECONDITION_FAILED'];
        var harness = harnessNames[status.status] ||
                      ('UNKNOWN(' + status.status + ')');
        var head = 'WPT HARNESS ' + harness;
        if (status.message) head += ' | ' + squash(status.message);
        var lines = [head];
        var counts = { PASS: 0, FAIL: 0, TIMEOUT: 0, NOTRUN: 0,
                       PRECONDITION_FAILED: 0 };
        var subtests = [];
        for (var i = 0; i < tests.length; i++) {
            var t = tests[i];
            var st = subtestNames[t.status] || ('UNKNOWN(' + t.status + ')');
            if (counts[st] !== undefined) counts[st]++;
            var line = 'WPT ' + st + ' ' + squash(t.name);
            if (t.message) line += ' | ' + squash(t.message);
            lines.push(line);
            subtests.push({
                name: t.name,
                status: st,
                message: t.message === undefined ? null : t.message
            });
        }
        lines.push('WPT SUMMARY total=' + tests.length +
                   ' pass=' + counts.PASS +
                   ' fail=' + counts.FAIL +
                   ' timeout=' + counts.TIMEOUT +
                   ' notrun=' + counts.NOTRUN +
                   ' precondition_failed=' + counts.PRECONDITION_FAILED);
        global.__ns_wpt_report = lines.join('\n') + '\n';
        global.__ns_wpt_json = JSON.stringify({
            harness: harness,
            message: status.message === undefined ? null : status.message,
            subtests: subtests
        });
        var harnessFailed = harness !== 'OK' &&
                            harness !== 'PRECONDITION_FAILED';
        global.__ns_wpt_failures = counts.FAIL + counts.TIMEOUT +
                                   counts.NOTRUN + (harnessFailed ? 1 : 0);
        global.__ns_wpt_done = true;
    };

    function arm(fn) {
        if (global.__ns_wpt_seen_harness) return;
        try {
            fn(global.__ns_wpt_oncomplete);
            global.__ns_wpt_seen_harness = true;
        } catch (e) {}
    }

    Object.defineProperty(global, 'add_completion_callback', {
        configurable: true,
        get: function () { return undefined; },
        set: function (fn) {
            delete global.add_completion_callback;
            global.add_completion_callback = fn;
            if (typeof fn !== 'function') return;
            arm(fn);
            if (!global.__ns_wpt_seen_harness) {
                Promise.resolve().then(function () { arm(fn); });
            }
        }
    });
})(typeof globalThis !== 'undefined' ? globalThis : this);
