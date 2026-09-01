# Private build configuration

`my_databank_entries.inc` contains the local Databank entries compiled into the
firmware. It is intentionally ignored by Git.

To configure a new checkout:

```sh
cp private/my_databank_entries.example.inc private/my_databank_entries.inc
```

Edit the copied file using C initializer rows in this format:

```c
    {"NAME", "VALUE"},
```

Names should be no longer than five characters. At least one entry is required.
The ordinary build will fail clearly if the private file has not been created.

These values are excluded from source control, but they are necessarily present
inside compiled firmware files. Do not publish a personal firmware binary.
