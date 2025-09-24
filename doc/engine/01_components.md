
# Components
Components are declared thru C-macros.

Components will consist of the following parameters:
- sparse[int32]
- dense[int32]
- data[T]
- size[ssize]
- capacity[ssize]

Following the idea of a sparse and dense array, the following
are mapped:
sparse[id] = index
dense[index] = id

## Related Macros
- ComponentDeclare(name, T) | src/game.h, line 10
    This is a convenience macro that declares a new struct named as the
    concatenation of a name `T` and "Storage". Ex. MovementStorage, etc.
    **Usage**: call globally, preferably a header file.
- ComponentCreate(storage, mem, size) | src/game.h, line 19
    This macro initializes the given component storage `storage`.
    `mem` refers to struct of function pointers used for allocation,
    which assumes the following parameters { make, free, realloc, ctx } exists.
    `size` refers to how much of each data is stored.
    This macro also initializes the array after allocation:
    { sparse = -1, dense = -1, data = 0 }
    **Usage**: call locally; *mandatory* before running functions/macros below.
    **Note**: Size is fixed.
- ComponentAdd(storage, id) | src/game.h, line 30
    This macro adds the given `id` of type `int32`, and asserts if the component
    storage is full.
    **Usage**: use after ComponentCreate for the specific component storage.
- ComponentArgs(storage, id, ...) | src/game.h, line 39
    This macro sets the initial values found with given `id` of type `int32`.
    **Usage**: use after ComponentAdd.
- ComponentAddArgs(storage, id, ...) | src/game.h, line 44
    This is a convenience macro that performs ComponentAdd and ComponentArgs
    simultaneously.
    **Usage**: use after ComponentCreate for the specific component storage.
- ComponentDelete(storage, id, count_ptr) | src/game.h, line 50
    This macro zeros-out the component of given `id` of type `int32`,
    and does not free any memory.
    **Warning**: this macro is untested, bounds check is not performed on given `id`,
    and can cause **overflow** to occur. ALSO avoid calling multiple times on the
    same id as this call decrements the component storage's size indiscriminately.
    **Usage**: use after ComponentAdd/Args.
