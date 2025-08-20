================
Style Guidelines
================

Python Code
-----------

* Generally follow Python's :pep:`8` style guidelines. We run Ruff to format
  our code which should help ensure consistent style. Make sure to run the
  following prior to committing changes:

  .. code-block:: python

     pre-commit install

  You can also check all files prior to a commit by running:

  .. code-block:: python

     pre-commit run --all-files

* Break up logical blocks of related code with a newline. Specifically add a
  blank newline after conditional or looping blocks.
* Don't comment what is obvious. Instead prefer meaningful names of functions
  and variables:

  .. code:: python

        # Get the functions signal annotations <-- this comment is unnecessary
        return_type, arg_types = get_signal_annotations(func)

* Use comments to explain non-obvious blocks and conditionals, magic,
  workarounds (with bug references), or generally complex pieces of code.
  Good examples:

  .. code:: python

        # If a property was defined with a decorator, it may already have
        # a name; if it was defined with an assignment (prop = Property(...))
        # we set the property's name to the member name
        if not prop.name:
            prop.name = name

  .. code:: python

        # Python causes MRO's to be calculated starting with the lowest
        # base class and working towards the descendant, storing the result
        # in __mro__ at each point. Therefore at this point we know that
        # we already have our base class MRO's available to us, there is
        # no need for us to (re)calculate them.
        if hasattr(base, '__mro__'):
            bases_of_subclasses += [list(base.__mro__)]


Python Doc Strings
------------------

* Doc strings should generally follow
  :pep:`257` unless noted here.
* Use `reStructuredText (reST) <https://www.sphinx-doc.org/en/master/usage/restructuredtext/>`__
  annotations.
* Use three double quotes for doc strings (``"""``).
* Use a brief description on the same line as the triple quote.
* Include function parameter documentation (including types, returns, and
  raises) between the brief description and the full description. Use a
  newline with indentation for the parameters descriptions.

  .. code:: python

        def spam(amount):
            """Creates a Spam object with the given amount.

            :param int amount:
                The amount of spam.
            :returns:
                A new Spam instance with the given amount set.
            :rtype: Spam
            :raises ValueError:
                If amount is not a numeric type.

            More complete description.
            """

* For class documentation, use the classes doc string for an explanation of
  what the class is used for and how it works, including Python examples.
  Include ``__init__`` argument documentation after the brief description in
  the classes doc string. The class ``__init__`` should generally be the first
  method defined in a class putting it as close as possible (location wise) to
  the class documentation.

  .. code:: python

        class Bacon(CookedFood):
            """Bacon is a breakfast food.

            :param CookingType cooking_type:
                Enum for the type of cooking to use.
            :param float cooking_time:
                Amount of time used to cook the Bacon in minutes.

            Use Bacon in combination with other breakfast foods for
            a complete breakfast. For example, combine Bacon with
            other items in a list to make a breakfast:

            .. code-block:: python

                breakfast = [Bacon(), Spam(), Spam(), Eggs()]

            """
            def __init__(self, cooking_type=CookingType.BAKE, cooking_time=15.0):
                super(Bacon, self).__init__(cooking_type, cooking_time)
