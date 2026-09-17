# Quxlang QA

## Will Quxlang implement collalescing?

Quxlang will not implement a Carbon-style collalescing phase. It was not observed to improve execution time in practice, and in theory destroys semantic information that can be used by the CPU branch predictor.