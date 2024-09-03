import inspect

def generate_signature(callable_info):
    return inspect.Signature.from_callable(lambda *args, **kwargs: None)
