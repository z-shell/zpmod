# Pull Request

## Description

<!-- Please include a summary of the change and which issue is fixed. Please also include relevant motivation and context. -->

Fixes # (issue)

## Type of change

<!-- Please check the relevant option by putting an `x` in the box -->

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Refactoring (no functional changes)
- [ ] Version bump/release preparation

## zpmod-Specific Testing

<!-- Please check all items that apply by putting an `x` in the box -->

- [ ] Module builds successfully (`make` or `./Scripts/install.sh`)
- [ ] Module loads without errors (`zmodload zi/zpmod`)
- [ ] Core functionality works:
  - [ ] Script compilation (`source` a .zsh file, check for .zwc creation)
  - [ ] Performance tracking (`zpmod source-study`)
  - [ ] Version command (`zpmod version`)
- [ ] Tested on target platforms:
  - [ ] Linux
  - [ ] macOS
  - [ ] Other Unix-like systems
- [ ] No memory leaks or file descriptor issues
- [ ] Compatibility with Zi plugin manager (if applicable)

## Version Management

<!-- Check if this PR affects versioning -->

- [ ] This PR requires a version bump
  - [ ] Patch version (bug fixes)
  - [ ] Minor version (new features)
  - [ ] Major version (breaking changes)
- [ ] Version updated using `./Scripts/bump-version.sh`
- [ ] CHANGELOG.md updated appropriately
- [ ] Documentation reflects version changes

## Checklist

<!-- Please check all items that apply by putting an `x` in the box -->

- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] I have run `./Scripts/update-readme.sh` to keep the README.md in sync with docs
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing tests pass locally with my changes
- [ ] Code follows the project's style guidelines
- [ ] I have checked for potential security implications

## Additional Information

<!-- Any additional information, configuration or data that might be necessary to reproduce the issue. -->

### Performance Impact

<!-- If applicable, describe any performance implications -->

### Breaking Changes

<!-- If this is a breaking change, describe what users need to do to migrate -->

### Dependencies

<!-- List any new dependencies or changes to existing dependencies -->
