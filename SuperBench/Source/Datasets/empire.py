import os
from itertools import combinations

from evaluation_metrics import tre, hausdorff, singularity_ratio, inverse_consistency_points, merge_dicts

class EMPIRE(object):
    # input_directory should point to the 'scans' directory. We do not use the lung masks.
    def __init__(self, input_directory):
        self.name = 'EMPIRE'
        self.category = 'Lung'

        self.image_file_names = []
        self.point_set_file_names = []
        self.deformation_field_file_names = []

        for i in range(1, 31):
            self.image_file_names.append((os.path.join(input_directory, 'scans', "%02d" % i + '_Fixed.mhd'),
                                          os.path.join(input_directory, 'scans', "%02d" % i + '_Moving.mhd')))

            # TODO: Find out output format
            self.deformation_field_file_names.append(("%02d" % i + '_Fixed_to_Moving.mhd',
                                                      "%02d" % i + '_Moving_to_Fixed.mhd'))


    def generator(self):
        for image_file_names, deformation_field_file_names in zip(self.image_file_names, self.deformation_field_file_names):
            yield {
                "image_file_names": image_file_names,
                "deformation_field_file_names": deformation_field_file_names
            }


    def evaluate(self, registration_driver, file_names):
        pass

