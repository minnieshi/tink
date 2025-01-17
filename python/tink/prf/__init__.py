# Copyright 2019 Google LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""PrfSet package."""

from tink.prf import _prf_key_manager
from tink.prf import _prf_key_templates as prf_key_templates
from tink.prf import _prf_set

Prf = _prf_set.Prf
PrfSet = _prf_set.PrfSet
register = _prf_key_manager.register
