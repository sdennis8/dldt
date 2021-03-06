"""
 Copyright (c) 2019 Intel Corporation

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""

import numpy as np

from mo.ops.op import Op


class ExperimentalDetectronDetectionOutput(Op):
    op = 'ExperimentalDetectronDetectionOutput'
    enabled = True

    def __init__(self, graph, attrs):
        mandatory_props = dict(
            type=__class__.op,
            op=__class__.op,
            infer=__class__.infer
        )

        super().__init__(graph, mandatory_props, attrs)

    def backend_attrs(self):
        return [
            'class_agnostic_box_regression',
            'max_detections_per_image',
            'nms_threshold',
            'num_classes',
            'post_nms_count',
            'score_threshold',
            'max_delta_log_wh',
            ('deltas_weights', lambda node: ','.join(map(str, node['deltas_weights'])))]

    @staticmethod
    def infer(node):
        rois_num = node.max_detections_per_image
        # boxes
        node.out_node(0).shape = np.array([rois_num, 4], dtype=np.int64)
        try:
            # classes
            node.out_node(1).shape = np.array([rois_num], dtype=np.int64)
            # scores
            node.out_node(2).shape = np.array([rois_num], dtype=np.int64)
            # batch_ids
            node.out_node(3).shape = np.array([rois_num], dtype=np.int64)
        except Exception as ex:
            print(ex)
