"""Small real-process gates for the parallel R2 owned-group lifecycle."""
import os
import signal
import subprocess
import sys
import unittest

import benchmark_full_weighted_parallel_r2 as driver


def launch(*, dead_leader=False, stubborn=False):
    child = "import signal,time; "
    if stubborn:
        child += "signal.signal(signal.SIGINT,signal.SIG_IGN);signal.signal(signal.SIGTERM,signal.SIG_IGN);"
    child += "print('ready',flush=True);time.sleep(60)"
    leader = ("import subprocess,sys,os,signal,time; "
              f"p=subprocess.Popen([sys.executable,'-c',{child!r}],stdout=subprocess.PIPE,text=True);"
              "p.stdout.readline();print(p.pid,flush=True);")
    leader += "os.kill(os.getpid(),signal.SIGKILL)" if dead_leader else "time.sleep(60)"
    process = subprocess.Popen([sys.executable, "-c", leader], start_new_session=True,
                               stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    child_pid = int(process.stdout.readline())
    process.stdout.close()
    return process, child_pid


class Lifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        driver.enable_subreaper()

    def check_closed(self, process, child):
        self.assertFalse(driver.group_exists(process.pid))
        with self.assertRaises(ProcessLookupError):
            os.kill(child, 0)

    def test_dead_leader_live_child_escalation(self):
        process, child = launch(dead_leader=True, stubborn=True)
        try:
            self.assertEqual(process.wait(timeout=3), -signal.SIGKILL)
            self.assertTrue(driver.group_exists(process.pid))
            result = driver.close_group(process)
            self.assertEqual(result["signals"], [signal.SIGINT, signal.SIGTERM, signal.SIGKILL])
            self.assertEqual(result["returncode"], -signal.SIGKILL)
            self.check_closed(process, child)
        finally:
            driver.close_group(process)

    def test_parent_keyboard_interrupt_cleans_group(self):
        process, child = launch()
        caught = False
        try:
            os.kill(os.getpid(), signal.SIGINT)
        except KeyboardInterrupt:
            caught = True
        finally:
            driver.close_group(process)
        self.assertTrue(caught)
        self.check_closed(process, child)

    def test_pending_interrupt_before_handle_registration(self):
        owned = {}
        process = None
        try:
            with self.assertRaises(KeyboardInterrupt):
                with driver.defer_signals():
                    process, child = launch()
                    os.kill(os.getpid(), signal.SIGINT)
                    owned[process.pid] = process
            self.assertIn(process.pid, owned)
        finally:
            if process is not None:
                driver.close_group(process)
        self.check_closed(process, child)

    def test_already_gone_group_idempotent(self):
        process = subprocess.Popen([sys.executable, "-c", "pass"], start_new_session=True)
        self.assertEqual(process.wait(timeout=3), 0)
        result = driver.close_group(process)
        self.assertFalse(result["residual_group_before_cleanup"])
        self.assertEqual(result["signals"], [])
        self.assertEqual(driver.close_group(process)["status"], "closed")


if __name__ == "__main__":
    unittest.main(verbosity=2)
