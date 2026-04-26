I used GitHub Copilot in Agent mode with Claude Haiku 4.5.

I created a file called `filter-requirements.md` where I described the context, what needed to be implemented, and a few implementation notes to guide the AI agent.

I then gave it the following prompt:

```text
"Generate a refined requirements instruction file based on filter-requirements.md. This file will be an input for a plan to implement the new features."
```

It generated the file `filter-implementation-spec.md`. I reviewed the generated requirements based on what it found, made a few changes to my initial requirements file, and corrected some mistakes. I repeated this process until I was satisfied with the result.

At first, it understood that it should also implement the `cmd_filer()` function. I had specifically mentioned that I did not want that function implemented.

Once I was happy with the refined requirements, I gave it the following prompt to generate an implementation action plan:

```text
"Generate an action plan based on filter-implementation-spec.md. is should be concise and straight to the point, giving explicit guidance for 
what to write and where. Append the information to a new file in the same package."
```

I reviewed the plan it generated in `filter-action-plan.md`. I did not find any mistakes, so I then gave it the following prompt to generate code based on that action plan:

```text
"Based on filter-action-plan.md generate the new functions."
```

I reviewed the generated code and was happy with the result. I learned that AI-generated output still needs to be reviewed carefully, because mistakes can be easy to miss.
