import sys

def remove_end_loop(buf):
    """
    >>> remove_end_loop(['abc', 'de', 'fg', 'fg', 'fg'])
    ['abc', 'de', 'fg']
    >>> remove_end_loop(['abc', 'de', 'fg', 'hi', 'fg', 'hi', 'fg', 'hi'])
    ['abc', 'de', 'fg', 'hi']
    >>> remove_end_loop(['abc', 'de', 'fg', 'hi', 'fg', 'hi', 'fg'])
    ['abc', 'de', 'fg', 'hi']
    >>> remove_end_loop(['abc', 'de', 'fg', 'hi', 'fg', 'hi'])
    ['abc', 'de', 'fg', 'hi']
    >>> remove_end_loop(['abc', 'de', 'fg', 'hi', 'fg'])
    ['abc', 'de', 'fg', 'hi']
    >>> remove_end_loop(['abc', 'de', 'fg', 'hi'])
    ['abc', 'de', 'fg', 'hi']
    >>> remove_end_loop(['abc', 'de', 'abc'])
    ['abc', 'de']
    >>> remove_end_loop(['fg', 'fg', 'fg'])
    ['fg']
    >>> remove_end_loop(['ab', 'ab', 'cd'])
    ['ab', 'ab', 'cd']
    >>> remove_end_loop(['ab', 'cd'])
    ['ab', 'cd']
    >>> remove_end_loop(['ab', 'ab'])
    ['ab']
    >>> remove_end_loop(['ab'])
    ['ab']
    >>> remove_end_loop(['a', '1', '2', '3', '1', '2', '3'])
    ['a', '1', '2', '3']
    >>> remove_end_loop(['a', '1', '2', '3', '1', '2'])
    ['a', '1', '2', '3']
    >>> remove_end_loop(['a', '1', '2', '3', '1'])
    ['a', '1', '2', '3']
    >>> remove_end_loop(['a', '1', '2', '3'])
    ['a', '1', '2', '3']
    >>> remove_end_loop(['a', '1', '2'])
    ['a', '1', '2']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3', '4', '1', '2', '3', '4'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3', '4', '1', '2', '3'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3', '4', '1', '2'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3', '4', '1'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3', '4'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2', '3'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1', '2'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4', '1'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3', '4'])
    ['a', '1', '2', '3', '4']
    >>> remove_end_loop(['a', '1', '2', '3'])
    ['a', '1', '2', '3']
    >>> remove_end_loop(['a', '1', '2'])
    ['a', '1', '2']
    >>> remove_end_loop(['a', '1', '2', '3', '1', '2', '1', '2', '1', '2', '3', '1', '2', '1', '2'])
    ['a', '1', '2', '3', '1', '2', '1', '2']
    >>> remove_end_loop(['a', '1', '2', '3', '1', '2', '1', '2', '1', '2', '3', '1', '2', '1', '2', '1', '2'])
    ['a', '1', '2', '3', '1', '2', '1', '2']
    >>> remove_end_loop([])
    []
    """

    # fixme: less convoluted way to do the same?
    sol = buf
    for s in range(1, 32):
        i0 = len(buf) - s
        i = i0
        last = buf[-s:]
        if len(last) != s: continue
        while i >= 0 and buf[i:i+s] == last:
            i -= s
        i += s
        if s > 1:
            # try to remove an extended version of the last repeating block
            # e.g. abc123412341 has "1234" repeating, so the result should be abc1234
            # last repeating block will be 2341, and we can remove one duplicate => abc12341
            # but we can go even further and expand 2341 to 12341, and remove that one instead
            # this trick ensures repeatability no matter where the last infinite loop ends
            k = i
            for j in range(1,s):
                lastx = last[-j:] + last
                if buf[k-j:k+s] == lastx:
                    i = k - j

        # pick the shortest solution
        if i+s < len(sol):
            sol = buf[:i+s]
    return sol

import doctest
doctest.testmod()

buf = list(sys.stdin.readlines())
buf = remove_end_loop(buf)
sys.stdout.write("".join(buf))
