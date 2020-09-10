# Don't import KernelTests
import jupyter_kernel_test
import unittest
import jupyter_kernel_mgmt
from jupyter_kernel_mgmt.discovery import KernelFinder
from jupyter_kernel_mgmt.client import BlockingKernelClient
import pytest


class RocketJoePythonTests(jupyter_kernel_test.KernelTests):
    kernel_name = 'rocketjoe'
    language_name = 'python'
    file_extension = '.py'
    code_hello_world = 'print(\'hello, world\')'
    code_stderr = 'import sys; print(\'test\', file = sys.stderr)'
    completion_samples = [{
        'text': 'zi', 'matches': {'zip'},
    }]
    complete_code_samples = ['1', 'print(\'hello, world\')', 'def f(x):\n  return x * 2\n']
    incomplete_code_samples = ['print(\'\'\'hello', 'def f(x):\n  x*2']
    invalid_code_samples = ['import = 7q']
    # code_page_something = 'zip?'
    code_generate_error = 'raise'
    code_execute_result = [
        {'code': '1 + 2 + 3', 'result': '6'},
        {'code': '[n * n for n in range(1, 4)]', 'result': '[1, 4, 9]'}
    ]
    # code_display_data = [
    #    {'code': 'from IPython.display import HTML, display; display(HTML(\'<b>test</b>\'))',
    #     'mime': "text/html"},
    #    {'code': 'from IPython.display import Math, display; display(Math(\'\\frac{1}{2}\'))',
    #     'mime': 'text/latex'}
    # ]
    code_history_pattern = '1?2*'
    # supported_history_operations = ('tail', 'search')
    code_inspect_sample = 'zip'


# def kernel_info_request(kc) -> None:
#     kc.kernel_info()
#
#
# def execute_request_1(kc) -> None:
#     kc.execute_interactive("i = 0\ni + 1")
#
#
# def execute_request_2(kc) -> None:
#     kc.execute_interactive("print(6 * 7)")
#
#
# def complete_request_1(kc) -> None:
#     kc.complete("prin")
#
#
# def complete_request_2(kc) -> None:
#     kc.complete("print")
#
#
# def is_complete_request_1(kc) -> None:
#     kc.is_complete("prin")
#
#
# def is_complete_request_2(kc) -> None:
#     kc.is_complete("print")
#
#
# class TestRocketjoe:
#     km = None
#     kc = None
#
#     @pytest.fixture
#     def start_a_kernel(self):
#         km, kc = jupyter_kernel_mgmt.start_kernel_blocking('spec/rocketjoe')
#         self.km = km
#         self.kc = kc
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_kernel_info_request(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#         await self.kc.kernel_info()
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_execute_request_1(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # execute_request_1(kc)
#         await self.kc.execute("i = 0\ni + 1")
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_execute_request_2(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # execute_request_2(kc)
#         await self.kc.execute("print(6 * 7)")
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_complete_request_1(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # complete_request_1(kc)
#         await self.kc.complete("prin")
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_complete_request_2(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # complete_request_2(kc)
#         await self.kc.complete("print")
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_is_complete_request_1(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # is_complete_request_1(kc)
#         await self.kc.is_complete("prin")
#
#     @pytest.mark.asyncio
#     async def test_rocketjoe_is_complete_request_2(self) -> None:
#         # async with jupyter_kernel_mgmt.run_kernel_async('rocketjoe') as kc:
#             # is_complete_request_2(kc)
#         await self.kc.is_complete("print")


if __name__ == '__main__':
    unittest.main()
